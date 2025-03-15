import React from "react";
import './Battery.css';

const Battery = ({ voltage, percentage }) => {

  // Dynamic styles for the battery fill
  const batteryStyle = {
    width: `${percentage}%`, // Fills battery based on percentage
    backgroundColor: `hsl(${percentage * 1.2}, 100%, 40%)`, // Green when full, red when low
    transition: "height 0.3s ease-in-out",
  };

  return (
    <div className="Battery">
      <div className="Battery-text w-100 text-center d-flex flex-column justify-content-center">
        <span className="fs-2 bold">{ percentage } %</span>
        <span>{typeof voltage === "number" ? voltage.toFixed(2) : voltage} V</span>
        
      </div>
      <div className="Battery-full" style={batteryStyle}></div>
    </div>
  );
};

export default Battery;
