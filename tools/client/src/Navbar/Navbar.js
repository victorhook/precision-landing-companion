import React from "react";
import './Navbar.css';

import Battery from './Battery/Battery';
import FlightMode from './FlightMode/FlightMode';
import Receiver from './Receiver/Receiver';


const Navbar = ({ status }) => {
  return (
    <div className="Navbar w-100 d-flex flex-row align-items-center">
      <Receiver rssi={status.ap_rssi}/>
      <div className="position-absolute start-50 translate-middle-x">
        <FlightMode flightMode={status.ap_flight_mode}/>
      </div>
      <div className="ms-auto">
        <Battery voltage={status.ap_bat_voltage} percentage={status.ap_bat_perc}/>
      </div>
    </div>
  );
};

export default Navbar;
