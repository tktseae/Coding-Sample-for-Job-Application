import axios from 'axios';
// import { Link } from 'react-router-dom';
import { usePredict } from '../utils/socket';
import LinearProgress from '@mui/material/LinearProgress';
import { useState } from 'react';
import { Box, Button, Grid, TextField, Typography } from '@mui/material';
import { LoadingButton } from '@mui/lab';
import { UploadFile, Download } from '@mui/icons-material';
import EnhancedTable from './EnhancedTable.js'

const FileUploader = () => {
  const [isloading, setIsLoading] = useState(false);
  const [fileAttached, setFileAttached] = useState(false);
  const { progress, setProgress, results, setResults } = usePredict();

  const [response, setResponse] = useState(null);

  const handleUpload = (e) => {
    e.preventDefault();
    console.log("Upload Clicked");

    setIsLoading(true);
    setProgress("0");
    setResults([]);
    setResponse(null);

    const file = e.target[0].files[0]
    const formData = new FormData();
    formData.append('file', file);

    axios.post('/upload', formData)
      .then(response => {
        console.log(response);
        setResponse(response);
        setIsLoading(false);
      })
      .catch(error => {
        console.error(error);
        setIsLoading(false);
      });
  };

  const handleDownload = (e) => {
    e.preventDefault();
    console.log("Download Clicked");
    if (response === null) return;
    const url = window.URL.createObjectURL(new Blob(["\uFEFF"+response.data], {type : 'text/csv; charset=utf-8'})) ;
    const link = document.createElement('a');
    link.href = url;
    link.setAttribute('download', "pred.csv");
    document.body.appendChild(link);
    link.click();
  };

  return ( 
    <div className="file-uploader">
      <Box
        component="form"
        method="post"
        encType="multipart/form-data"
        onSubmit={handleUpload}
      >
        <Grid container direction="column" alignItems="center" spacing={3}>
          <Grid item>
            <Typography variant='h4' align='center' fontWeight={500}>
              Psychosocial Wellness Assessment via Conversation
            </Typography>
          </Grid>
          {/* File input */}
          <Grid item>
            <TextField 
              type="file" 
              accept="audio/*" 
              disabled={isloading} 
              onChange={() => setFileAttached(true)} 
            />
          </Grid>

          {/* Upload and Download Buttons */}
          <Grid item>
            <LoadingButton 
              type="submit" 
              variant="contained" 
              color="primary" 
              loadingPosition="start" 
              disabled={!fileAttached} 
              loading={isloading} 
              startIcon={<UploadFile />} 
              sx={{width:150, mr:1}}
            >
              {!isloading ? "Upload" : "Processing..." }
            </LoadingButton>
            <Button 
              variant="contained" 
              startIcon={<Download />} 
              sx={{width:150, ml:1}} 
              disabled={response === null}
              onClick={handleDownload}
            >
              Download
            </Button>
          </Grid>

          {/* Progress */}
          { fileAttached &&
            <Grid item>
              <Box display='flex'>
                <Box width='50vw' alignContent='center'>
                  <LinearProgress variant="determinate" value={Number(progress)} sx={{ height: 10, borderRadius: 5}} />
                </Box>
                <Box>
                  <Typography variant='body1' ml={3}>{progress} %</Typography>
                </Box>
              </Box>
            </Grid>
          }

          {/* Prediction Table */}
          <Grid item>
            <Box sx={{ width:'95vw'}}>
              <EnhancedTable response={response} results={results} />
            </Box>
          </Grid>
        </Grid>
      </Box>
    </div>
  );
}
 
export default FileUploader;