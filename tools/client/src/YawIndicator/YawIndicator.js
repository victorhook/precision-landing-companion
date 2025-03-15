import React, { useRef, useEffect } from "react";

const YawIndicator = ({ yaw }) => {
  const canvasRef = useRef(null);

  useEffect(() => {
    const canvas = canvasRef.current;
    const ctx = canvas.getContext("2d");

    const width = canvas.width;
    const height = canvas.height;
    const centerX = width / 2;
    const maxShift = width / 2; // Max shift in pixels

    ctx.clearRect(0, 0, width, height);

    // Normalize yaw to -180 to 180 range
    let normalizedYaw = yaw % 360;
    if (normalizedYaw > 180) normalizedYaw -= 360;
    if (normalizedYaw < -180) normalizedYaw += 360;

    // Compute stripe movement (continuous scrolling effect)
    const yawShift = (normalizedYaw / 180) * maxShift; // Convert yaw angle to pixel shift
    const stripeWidth = 10;
    const stripeSpacing = 20;

    ctx.save();
    ctx.translate(centerX + yawShift, height / 2);

    // Draw repeating stripes to handle wrapping
    ctx.fillStyle = "white";
    for (let i = -10; i <= 10; i++) {
      const stripeX = (i * stripeSpacing) % width; // Repeat stripes continuously
      ctx.fillRect(stripeX - stripeWidth / 2, -5, stripeWidth, 10);
    }

    ctx.restore();

    // Draw Yaw Angle Text
    ctx.fillStyle = "white";
    ctx.font = "14px Arial";
    ctx.textAlign = "center";
    ctx.fillText(`${normalizedYaw.toFixed(1)}°`, centerX, height);
  }, [yaw]);

  return <canvas ref={canvasRef} width={300} height={50} />;
};

export default YawIndicator;
