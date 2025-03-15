import React, {useState, useEffect, useMemo} from "react";
import './RadioChannels.css';

const PWM_MIN = 1000
const PWM_MAX = 2000

const RadioChannel = ({ channel, value }) => {

  const [width, setWidth] = useState(50);

  useEffect(() => {
    let ratio = (value - PWM_MIN) / (PWM_MAX - PWM_MIN);
    setWidth(ratio * 100)
  }, [value]);


  return (
    <div className="RadioChannel d-flex m-2 text-center align-item-center">
        <span className="RadioChannelNumber fs-5">
          { channel }
        </span>
        <div className="RadioChannelBar">
          <span className="fs-5">
            { value }
          </span>
          <div className="RadioChannelBarFill" style={{width: `${width}%`}}></div>
        </div>
    </div>
  );
};

const RadioChannels = ({ status }) => {

  const channels = useMemo(() => {
    return Object.entries(status).filter(([key]) => key.startsWith("ap_chan"));
  }, [status]);
  

  const channelNumber = channelString => {
    return parseInt(channelString.match(/\d+/), 10);
  }

  return (
    <div className="RadioChannels text-center">
      {
        channels.map(([channel, value]) =>
        <RadioChannel key={channel} channel={channelNumber(channel)} value={value}></RadioChannel>
      )
      }
    </div>
  );
};

export default RadioChannels;
