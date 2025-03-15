import React, { useState, useEffect } from "react";
import RealTimeGraph from "../RealTimeGraph/RealTimeGraph";
import './LowerGraph.css';

const LowerGraph = ({ status }) => {

  const [sensorData, setSensorData] = useState({ rangeFinderDist: 0 });
/*
  useEffect(() => {
    const interval = setInterval(() => {
      setSensorData({
        value1: Math.random() * 100, // Random data for testing
        value2: Math.random() * 50,  // Another data source
      });
    }, 1000);

    return () => clearInterval(interval);
  }, []);*/

  useEffect(() => {
    setSensorData({rangeFinderDist: status.freeHeap});
  }, [status]);

  return (
    <div className="LowerGraph">
      <h2>Real-Time Graph</h2>
      <RealTimeGraph newData={sensorData} timeWindow={30} />
    </div>
  );
};

export default LowerGraph;
