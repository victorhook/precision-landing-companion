import React, { useState } from "react";
import './Button.css';
import Api from "../Api";
import { toast } from 'react-toastify';


const Button = ({ text, askIfSure = false, askUserIfSure }) => {
  const clickedOk = () => toast(`${text} OK`);
  const clickedError = () => toast(`${text} ERROR`);

  const onClick = async () => {
    let proceed = true;
    if (askIfSure) {
      console.log('Asking if ok!');
      proceed = await askUserIfSure();
    }

    if (proceed) {
      let result = await Api.setCommand(text);
      if (result && result.status == 'success') {
        clickedOk();
      } else {
        console.error(`Failed to send command ${text}`)
        clickedError();
      }
    }
  }

  return (
    <input
      className="Button fs-4"
      type="button"
      value={text}
      onClick={onClick}
    />
  );
};

export default Button;
