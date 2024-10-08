import { io } from 'socket.io-client';
import { useState, useEffect } from 'react';

const socket = io('http://127.0.0.1:5000', {
  autoConnect: false
});

const usePredict = () => {
  const [progress, setProgress] = useState(0);
  const [results, setResults] = useState([]);

  useEffect(() => {
    socket.on('predict', ({result, progress}) => {
      setProgress(progress);
      setResults(prevState => 
        [...prevState, createData(result[0], result[1], result[2], result[3], result[4])]);
    });
    socket.connect();

    return () => {
      socket.off('predict');
      socket.disconnect();
    };
  }, []);

  return { progress, setProgress, results, setResults }
}

function createData(index, start, end, transcription, label) {

  let labelInText = "";

  if (label === "normal") {
    label = 0;
    labelInText = "Normal";
  } else if (label === "mild") {
    label = 1;
    labelInText = "Mild";
  } else {
    label = 2;
    labelInText = "Severe";
  }

  return {
    index,
    start,
    end,
    transcription,
    label,
    labelInText,
  };
}

export {
  socket,
  usePredict,
}