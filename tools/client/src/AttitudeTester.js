import React, { useState } from "react";
import AttitudeIndicator from "./AttitudeIndicator/AttitudeIndicator"; // Main Indicator
import YawIndicator from "./YawIndicator/YawIndicator"; // Yaw at Bottom

const AttitudeTester = () => {
  const [roll, setRoll] = useState(0);
  const [pitch, setPitch] = useState(0);
  const [yaw, setYaw] = useState(0);

  return (
    <div style={{ textAlign: "center" }}>
      <h2>Attitude Indicator Tester</h2>

      {/* Attitude Indicator Display */}
      <AttitudeIndicator roll={roll} pitch={pitch} />

      {/* Yaw Indicator */}
      <YawIndicator yaw={yaw} />

      {/* Roll Slider */}
      <div>
        <label>Roll: {roll}°</label>
        <input
          type="range"
          min={-45}
          max={45}
          value={roll}
          onChange={(e) => setRoll(parseFloat(e.target.value))}
        />
      </div>

      {/* Pitch Slider */}
      <div>
        <label>Pitch: {pitch}°</label>
        <input
          type="range"
          min={-45}
          max={45}
          value={pitch}
          onChange={(e) => setPitch(parseFloat(e.target.value))}
        />
      </div>

      {/* Yaw Slider */}
      <div>
        <label>Yaw: {yaw}°</label>
        <input
          type="range"
          min={-180}
          max={180}
          value={yaw}
          onChange={(e) => setYaw(parseFloat(e.target.value))}
        />
      </div>
    </div>
  );
};

export default AttitudeTester;
