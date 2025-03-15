import React from "react";
import './VideoTop.css';
import arrowIcon from "./arrow-up.svg";


const VideoTop = ({ rangefinderDistance }) => {
  return (
    <div className="VideoTop d-flex justify-content-center">
      <div className="Rangefinder d-flex flex-row align-items-center">
        <span className="me-3">
          <img src={arrowIcon} alt="My Icon" width="50" height="50" />
        </span>
        <span className="fs-1 bold">
          {typeof rangefinderDistance === "number" ? rangefinderDistance.toFixed(2) : rangefinderDistance} m
        </span>
      </div>
    </div>
  );
};

export default VideoTop;
