#include "darknet.h"
#include "opencv2/opencv.hpp"

#include <iostream>
#include <zmq.hpp>
#include <unistd.h>
#include <sys/stat.h>

#define BLUE  0
#define GREEN 1
#define RED   2

#define STRSIZE 256
#define FILE    (char*)request.data()
#define PIXEL   (*pixel)

using namespace std;
using namespace zmq;
using namespace cv;

struct detected_obj
{
    int left, right, top, bot;
    // float prob;
};

int main(int argc, char **argv)
{
    // net setup
  	gpu_index = find_int_arg(argc, argv, "-i", 0);
  	if (find_arg(argc, argv, "-nogpu"))
		gpu_index = -1;

#ifndef GPU
  	gpu_index = -1;
#else
  	if(gpu_index >= 0)
    	cuda_set_device(gpu_index);
#endif

  	float thresh = find_float_arg(argc, argv, "-thresh", .5);
  	float hier_thresh = .5;
  	char *cfgfile = "include/yolo-obj.cfg";
  	char *weightfile = "include/yolo-obj_2000.weights";

    network *net = load_network(cfgfile, weightfile, 0);
    set_batch_network(net, 1);
    srand(2222222);

    //zeromq setup
    context_t context(1);
    socket_t socket(context, ZMQ_REP);
    socket.bind("tcp://*:5555");

    while (true)
    {
        char filename [STRSIZE] = "static/img/";
        char processed[STRSIZE] = "processed/";

        //  wait for next request from client
        message_t request;
        socket.recv(&request);

        strcat(filename, FILE);

        // check if file exists
        struct stat buffer;
        if (stat(filename, &buffer) != 0)
        {
            message_t reply(5);
            memcpy(reply.data(), "error", 5);
            socket.send(reply);
            continue;
        }

        // magic
      	image im = load_image_color(filename, 0, 0);
      	image sized = letterbox_image(im, net->w, net->h);

      	float *X = sized.data;
      	network_predict(net, X);
      	int nboxes = 0;

      	detection *dets = get_network_boxes(net, im.w, im.h, thresh, hier_thresh,
                                            0, 1, &nboxes);

        // store all detections in a struct
        detected_obj detected_objs[nboxes];

      	for (int i = 0; i < nboxes; i++)
      	{
    	    auto b = dets[i].bbox;

    	    int left  = (b.x - b.w / 2.) * im.w;
    	    int right = (b.x + b.w / 2.) * im.w;
    	    int top   = (b.y - b.h / 2.) * im.h;
    	    int bot   = (b.y + b.h / 2.) * im.h;

    	    if (left  < 0       ) left  = 0;
    	    if (right > im.w - 1) right = im.w - 1;
    	    if (top   < 0       ) top   = 0;
    	    if (bot   > im.h - 1) bot   = im.h - 1;

    	    detected_objs[i] = {left, right, top, bot}; //, dets[i].prob[0]};

    	    // cout << left << ' ' << right << ' ' << top << ' ' << bot << endl;
      	}

      	free_image(im);
      	free_image(sized);

        // process image
        Mat img = imread(filename);

        for (int i = 0; i < img.rows; i++)
            for (int j = 0; j < img.cols; j++)
            {
                // verify if pixel is part of a detected obj
                int k;

                for (k = 0; k < nboxes; k++)
                    if (detected_objs[k].top  <= i and i <= detected_objs[k].bot and
                        detected_objs[k].left <= j and j <= detected_objs[k].right)
                        break;

                if (k < nboxes)
                    break;

                // make pixel red if it is green
                auto *pixel = &img.at<Vec3b>(i, j);
                if (PIXEL[GREEN] > PIXEL[RED] and
                    PIXEL[GREEN] > PIXEL[BLUE])
                {
                    PIXEL[RED]   = min(PIXEL[RED] + 30, 255);
                    PIXEL[GREEN] = 0;
                    PIXEL[BLUE]  = 0;
                }
            }


        // save image
        strcat(processed, FILE);
        imwrite(processed, img);

        // send reply
        message_t reply(2);
        memcpy(reply.data(), "ok", 2);
        socket.send(reply);
    }
}
