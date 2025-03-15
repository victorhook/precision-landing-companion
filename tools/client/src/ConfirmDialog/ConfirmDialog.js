import React, { useState } from "react";
import './ConfirmDialog.css';


const ConfirmDialog = ({ message, onConfirm, onCancel }) => {
  return (
    <div className="ConfirmDialog">
      <div className="confirm-box">
        <p className="fs-1">{message}</p>
        <button className="Button bold fs-2 mx-3 my-2 p-3" onClick={onConfirm}>Yes</button>
        <button className="Button bold fs-2 mx-3 my-2 p-3" onClick={onCancel}>No</button>
      </div>
    </div>
  );
};

export default ConfirmDialog;
