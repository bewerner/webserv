from flask import Flask, request, jsonify, render_template
import os
from werkzeug.utils import secure_filename
import uuid

app = Flask(__name__)

# Configuration
UPLOAD_FOLDER = 'uploads'
MAX_CONTENT_LENGTH = 5 * 1024 * 1024  # 5MB max upload size
ALLOWED_EXTENSIONS = {'txt', 'pdf', 'png', 'jpg', 'jpeg', 'gif', 'doc', 'docx', 'xls', 'xlsx', 'zip'}

# Create upload folder if it doesn't exist
os.makedirs(UPLOAD_FOLDER, exist_ok=True)

app.config['UPLOAD_FOLDER'] = UPLOAD_FOLDER
app.config['MAX_CONTENT_LENGTH'] = MAX_CONTENT_LENGTH

def allowed_file(filename):
    return '.' in filename and filename.rsplit('.', 1)[1].lower() in ALLOWED_EXTENSIONS

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/upload', methods=['POST'])
def upload_file():
    # Check if the post request has the file part
    if 'fileToUpload' not in request.files:
        return jsonify({'success': False, 'message': 'No file part in the request'}), 400
    
    file = request.files['fileToUpload']
    
    # If user does not select file, browser also
    # submit an empty part without filename
    if file.filename == '':
        return jsonify({'success': False, 'message': 'No file selected'}), 400
    
    if file:
        # Secure the filename and add a UUID to avoid overwriting
        original_filename = secure_filename(file.filename)
        filename_parts = os.path.splitext(original_filename)
        unique_filename = f"{filename_parts[0]}_{str(uuid.uuid4())[:8]}{filename_parts[1]}"
        
        file_path = os.path.join(app.config['UPLOAD_FOLDER'], unique_filename)
        file.save(file_path)
        
        return jsonify({
            'success': True,
            'message': 'File uploaded successfully',
            'filename': unique_filename
        })
    
    return jsonify({'success': False, 'message': 'Unknown error occurred'}), 500

# Error handlers
@app.errorhandler(413)
def too_large(e):
    return jsonify({'success': False, 'message': 'File is too large (max 5MB)'}), 413

if __name__ == '__main__':
    app.run(debug=True)