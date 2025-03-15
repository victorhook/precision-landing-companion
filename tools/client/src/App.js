import "bootstrap/dist/css/bootstrap.min.css";
import "bootstrap/dist/js/bootstrap.bundle.min.js";
import './App.css';

import React, { createContext, useState, useEffect } from "react";

import Navbar from './Navbar/Navbar';
import LogConsole from './LogConsole/LogConsole';
import VideoTop from './VideoTop/VideoTop';
import Video from './Video/Video';
import LowerGraph from './LowerGraph/LowerGraph';
import Joystick from './Joystick/Joystick';
import AttitudeIndicator from './AttitudeIndicator/AttitudeIndicator';
import YawIndicator from './YawIndicator/YawIndicator';
import LocalMap from './LocalMap/LocalMap';
import SmallDataTable from './SmallDataTable/SmallDataTable';
import RadioChannels from './RadioChannels/RadioChannels';
import ButtonGroup from './ButtonGroup/ButtonGroup';
import ConfirmDialog from "./ConfirmDialog/ConfirmDialog";
import { ToastContainer, toast } from 'react-toastify';

import Data from './Data';
import Api from './Api';


function App() {
  const [status, setStatus] =  useState({
    ap_pitch: 0,
    ap_roll: 0,
    ap_yaw: 0,
    ap_rngfnd_dist_m: 0,
    ap_bat_voltage: 0,
    ap_bat_perc: 0,
    ap_local_pos_x: 0,
    ap_local_pos_y: 0,
    ap_local_pos_z: 0,
    ap_local_pos_vx: 0,
    ap_local_pos_vy: 0,
    ap_local_pos_vz: 0,
    ap_relative_alt: 0,
    ap_alt: 0,
    ap_chan1_raw: 1500,
    ap_chan2_raw: 1500,
    ap_chan3_raw: 1500,
    ap_chan4_raw: 1500,
    ap_chan5_raw: 1500,
    ap_chan6_raw: 1500,
    ap_chan7_raw: 1500,
    ap_chan8_raw: 1500,
    ap_chan9_raw: 1500,
    ap_chan10_raw: 1500,
    ap_rssi: -32,
    upTimeMs: 0,
  });
  const [data, setData] =  useState(Data);
  const [logs, setLogs] =  useState([]);
  const [confirmData, setConfirmData] = useState(null);

  // Function to show the modal and return user's choice
  const askUserIfSure = () => {
    return new Promise((resolve) => {
      setConfirmData({
        message: "Are you sure?",
        onConfirm: () => {
          setConfirmData(null);
          resolve(true);
        },
        onCancel: () => {
          setConfirmData(null);
          resolve(false);
        },
      });
    });
  };

  const handleTelemetryPacket = packet => {
    if (packet == null) {
      return;
    }
    switch (packet.type) {
      case "Log":
        setLogs(prevLogs => [...prevLogs, packet]);  // ✅ Uses the latest state
        break;
      case "Tags":
        break;
      case "Status":
        setStatus(packet);
        break;
      default:
        console.error(`Unknown telemetry packet type ${packet.type}`);
    }
  }

  useEffect(() => {
    let isMounted = true;
  
    const fetchTelemetry = async () => {
      const newPackets = await Api.getTelemetry();
      if (newPackets && isMounted) {
        for (let packet of newPackets) {
          handleTelemetryPacket(packet);
        }
      }
    };
  
    // ✅ Use setInterval instead of recursive setTimeout
    const interval = setInterval(fetchTelemetry, 50);
  
    return () => {
      isMounted = false;
      clearInterval(interval); // ✅ Properly stops fetching when unmounted
    };
  }, []);

  return (
    <div className='App container-fluid'>
      <ToastContainer />

      <div className='navbar'>
        <Navbar status={status}/>
      </div>
      <div className='MainScreen d-flex flex-row'>
          {confirmData && <ConfirmDialog {...confirmData} />}
          {/*
          <ConfirmDialog 
            message="Are you sure?" 
            onConfirm={() => {}}
            onCancel={() => {}}
          />
          */}

          <div className="LeftScreen d-flex flex-column">
            <LogConsole logs={logs} setLogs={setLogs}/>
            <div className="d-flex w-100">
              <RadioChannels status={status}/>
              <ButtonGroup askUserIfSure={askUserIfSure}/>
            </div>
          </div>
          
          <div className="MiddleScreen d-flex flex-column justify-content-center">

            <div className='row mb-3'>
              <span>REL ALT: {status.ap_relative_alt.toFixed(3)}m</span>
              <span>ALT: {status.ap_alt.toFixed(3)}m</span>
                <VideoTop rangefinderDistance={status.ap_rngfnd_dist_m}/>
            </div>
            <div className='row m-5'>
                <Video />
            </div>
            
            <div className='row'>
                <div className='col-3'>
                    <Joystick />
                </div>
                <div className='col-6'>
                    <LowerGraph status={status}/>
                </div>
                <div className='col-3'>
                    <Joystick />
                </div>
            </div>
          </div>

          <div className="RightScreen">
            <div className='row text-center p-4'>
                <div className="offset-2 col-8">
                <AttitudeIndicator roll={status.ap_roll} pitch={status.ap_pitch}/>
                <YawIndicator yaw={status.ap_yaw}/>
                </div>
            </div>
            <div className='row'>
                <LocalMap x={status.ap_local_pos_x} y={status.ap_local_pos_x} yaw={status.ap_yaw}/>
            </div>
            <div className='row'>
              <SmallDataTable
              data={status}
            />
            </div>
          </div>
      </div>
    </div>
  );
}

export default App;
