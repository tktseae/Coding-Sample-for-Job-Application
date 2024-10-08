import os
import shutil
import pandas as pd
from flask import Flask, request, send_file
from flask_socketio import SocketIO
from store import *
from transcribe import *
from cut_audio import *
from duration import *
from predict import *
# import time

app = Flask(__name__)
socketio = SocketIO(app, cors_allowed_origins=["http://localhost:3000"])

SAVE_AND_DELETE=True
TEST_AUDIO_DIRS=""
TEST_AUDIO_PATH=""

@app.route("/upload", methods=["GET", "POST"])
def upload():
  file = request.files.get("file")
  file_name = file.filename

  # If you do NOT want to keep creating new audio and deleting it
  # set the variable SAVE_AND_DELETE=False
  # also define the TEST_AUDIO_DIRS and TEST_AUDIO_PATH
  if SAVE_AND_DELETE:
    file_directory, file_path = store(file)
  else:
    file_directory = TEST_AUDIO_DIRS
    file_path = TEST_AUDIO_PATH

  results_df = pd.DataFrame(columns=['start', 'end', 'transcription', 'label'])
  duration = get_duration(file_path)

  segments = transcribe(file_path)
  for index, segment in enumerate(segments):
    progress = round(segment.end/duration*100)
    print("%i%% [%.2fs -> %.2fs] %s" % (progress, segment.start, segment.end, segment.text))

    # cut audio
    # file_segment_path is "record0_0.wav" and store at /uploads/record0 folder
    file_segment_path = cut_audio(file_directory, file_path, file_name, 
                                  segment.start, segment.end, index)
    # print(file_segment_path)

    # predict
    pred_label = predict(file_segment_path)
    print("pred_label is", pred_label)

    results_df.loc[len(results_df)] = [segment.start, segment.end, segment.text, pred_label]
    socketio.emit("predict", {"result": [index, segment.start, segment.end, segment.text, pred_label], 
                              "progress": progress})

  # for i in range(5):
  #   time.sleep(1)
  #   result = [i, i, i+1, f"{i}Output", "normal"]
  #   results_df.loc[len(results_df)] = [i, i+1, f"{i}Output", "normal"]
  #   socketio.emit("predict", {"result": result, 
  #                             "progress": int((i+1)/5*100)})

  csv_path = os.path.join(file_directory, "pred.csv")
  results_df.to_csv(csv_path, index=False)

  response = send_file(
    path_or_file=csv_path,
    mimetype='text/csv',
    as_attachment=True,
    download_name='pred.csv'
  )

  # Remove the uploaded file and all its intermediate files 
  # if SAVE_AND_DELETE:
  #   shutil.rmtree(file_directory, ignore_errors=False, onerror=None)

  return response

# @app.route("/download/<download_id>", methods=["GET"])
# def download():
#   return

@socketio.on('disconnect')
def disconnect():
  socketio.emit('disconnect', {'message': 'SEVER Disconnected'})

if __name__ == "__main__":
  socketio.run(app, debug=True, port=5000)
