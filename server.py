import numpy as np
import zmq
from PIL import Image
from flask import Flask, render_template, request, send_file
from flask_uploads import UploadSet, configure_uploads, IMAGES

# flask setup
app = Flask(__name__)
photos = UploadSet('photos', IMAGES)
app.config['UPLOADED_PHOTOS_DEST'] = 'static/img'
configure_uploads(app, photos)

# zmq setup
context = zmq.Context()
socket = context.socket(zmq.REQ)
socket.connect("tcp://localhost:5555")

@app.route('/', methods=['GET', 'POST'])
def upload():
    if request.method == 'POST' and 'photo' in request.files:
        filename = photos.save(request.files['photo'])

        filename_bytes = filename.encode('ascii') + b'\0'
        socket.send(filename_bytes)
        message = socket.recv()

        if message != b'ok':
            return 'Error'

        list_im   = ['processed/' + filename, 'static/img/' + filename]
        imgs      = [Image.open(i) for i in list_im]
        min_shape = sorted([(np.sum(i.size), i.size ) for i in imgs])[0][1]
        imgs_comb = np.hstack((np.asarray(i.resize(min_shape)) for i in imgs))
        imgs_comb = Image.fromarray(imgs_comb)
        imgs_comb.save('processed/' + filename)

        return send_file('processed/' + filename)

    return render_template('upload.html')

if __name__ == '__main__':
    app.run(debug = True)
