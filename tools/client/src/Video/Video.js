import React from "react";
import './Video.css';

const Video = () => {
  return (
    <div className="Video text-center">
      <img src="http://localhost:8080/video" alt="Drone Feed" className="video-stream" />
    </div>
  );
};

export default Video;
