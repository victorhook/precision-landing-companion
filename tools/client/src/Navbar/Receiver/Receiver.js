import React, { useRef, useState, useEffect } from "react";
import './Receiver.css';


import Rssi1 from "./rssi_1.svg";
import Rssi2 from "./rssi_2.svg";
import Rssi3 from "./rssi_3.svg";

const Receiver = ({ rssi }) => {
  
  const [imageSrc, setImageSrc] = useState(Rssi1);

  const getImage = rssiValue => {
    return Rssi1;
  }

  useEffect(() => {
    let img = getImage(rssi)
    if (img) {
      setImageSrc(img); // Update only if rssi changes
    }
  }, [rssi]); // Runs only when `rssi` changes

  return (
    <div className="NavbarItem Receiver d-flex flex-row">
        <img className="RssiImage" src={imageSrc} alt='Dynamic' />
        <span className="ms-2 me-2 fs-4">{ rssi }</span>
    </div>
  );
};

export default Receiver;
