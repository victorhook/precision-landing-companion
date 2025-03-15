import React, {useState, useEffect, useMemo} from "react";
import "./Joystick.css";
import JoystickBase from "./joystick-base.png"; // Background
import JoystickTop from "./joystick-top.png"; // Movable knob

const mapRange = (value, inMin, inMax, outMin, outMax) => {
  return ((value - inMin) * (outMax - outMin)) / (inMax - inMin) + outMin;
};

const Joystick = ({ x = 1500, y = 1500 }) => {
  // Ensure values stay within range
  const mappedX = useMemo(() => mapRange(Math.min(Math.max(x, 988), 2012), 1000, 2000, -30, 30), [x]);
  const mappedY = useMemo(() => mapRange(Math.min(Math.max(y, 988), 2012), 1000, 2000, -30, 30), [y]);

  return (
    <div className="Joystick">
      {/* Background (Fixed) */}
      <img src={JoystickBase} alt="Joystick Base" className="joystick-base" />

      {/* Movable Joystick */}
      <img
        src={JoystickTop}
        alt="Joystick Top"
        className="joystick-top"
        style={{
          transform: `translate(${mappedX}px, ${-mappedY}px)`, // Invert Y for correct movement
        }}
      />
    </div>
  );
};

export default Joystick;
