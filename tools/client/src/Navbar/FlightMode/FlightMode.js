import React, {useMemo} from "react";
import './FlightMode.css';


const FLIGHT_MODES = {
  0: "Stabilize",
  1: "Acro",
  2: "AltHold",
  3: "Auto",
  4: "Guided",
  5: "Loiter",
  6: "RTL",
  7: "Circle",
  9: "Land",
  11: "Drift",
  13: "Sport",
  14: "Flip",
  15: "AutoTune",
  16: "PosHold",
  17: "Brake",
  18: "Throw",
  19: "Avoid_ADSB",
  20: "Guided_NoGPS",
  21: "Smart_RTL",
  22: "FlowHold",
  23: "Follow",
  24: "ZigZag",
  25: "SystemID",
  26: "Heli_Autorotate",
  27: "Auto RTL"
};


const FlightMode = ({ flightMode = 255 }) => {
  const flightModeString = useMemo(() => {
    return FLIGHT_MODES[flightMode] || "UNKNOWN";
  }, [flightMode]);

  return (
    <div className="NavbarItem FlightMode">
        <h1>
          {flightModeString} {flightModeString === "UNKNOWN" ? `(${flightMode})` : ""}
        </h1>
    </div>
  );
};

export default FlightMode;
