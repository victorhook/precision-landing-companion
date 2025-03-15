import React from "react";
import './ButtonGroup.css';
import Button from '../Button/Button';


const ButtonGroup = ({ askUserIfSure }) => {
  return (
    <div className="ButtonGroup d-flex flex-column p-2">
        <Button text={'ArmCheck'} askUserIfSure={askUserIfSure}/>
        <Button text={'Arm'} askUserIfSure={askUserIfSure}/>
        <Button text={'Disarm'} askUserIfSure={askUserIfSure}/>
        <Button text={'TakeOff'} askUserIfSure={askUserIfSure}/>
        <Button text={'Land'} askUserIfSure={askUserIfSure}/>
        <Button text={'Reboot'} askUserIfSure={askUserIfSure} askIfSure={true}/>
        <Button text={'Reboot AP'} askUserIfSure={askUserIfSure} askIfSure={true}/>
    </div>
  );
};

export default ButtonGroup;
