import React, { useRef, useEffect } from "react";
import FrameImage from "./frame.svg"; // Frame
import ArrowImage from "./horizon-arrow.svg"; // Arrow icon (small)

const AttitudeIndicator = ({ roll, pitch }) => {
  const canvasRef = useRef(null);
  const frameRef = useRef(new Image());
  const arrowRef = useRef(new Image());

  useEffect(() => {
    const canvas = canvasRef.current;
    const ctx = canvas.getContext("2d");

    const drawAttitudeIndicator = () => {
      const width = canvas.width;
      const height = canvas.height;
      const centerX = width / 2;
      const centerY = height / 2;
      const horizonWidth = width * 3; // Extra-wide for smooth rotation
      const horizonHeight = height * 2; // Taller to allow pitch movement

      ctx.clearRect(0, 0, width, height);

      // **Clip a circle** before drawing
      ctx.save();
      ctx.beginPath();
      ctx.arc(centerX, centerY, (width / 2)-4, 0, Math.PI * 2);
      ctx.clip();

      // Compute pitch offset
      const pitchOffset = (pitch / 50) * (height / 2); // Scale pitch (-50 to 50)

      // Draw Horizon
      ctx.save();
      ctx.translate(centerX, centerY + pitchOffset);
      ctx.rotate((roll * Math.PI) / 180); // Apply roll rotation

      // Draw Sky
      ctx.fillStyle = "#007bff"; // Blue sky
      ctx.fillRect(-horizonWidth / 2, -horizonHeight / 2, horizonWidth, horizonHeight / 2);

      // Draw Ground
      ctx.fillStyle = "#8B4513"; // Brown ground
      ctx.fillRect(-horizonWidth / 2, 0, horizonWidth, horizonHeight / 2);

      ctx.restore();

      // Draw Arrow (follows roll but stays at top)
      ctx.save();
      ctx.translate(centerX, 40); // Keep at top-center of frame
      ctx.rotate((roll * Math.PI) / 180); // Rotate same as horizon
      ctx.drawImage(arrowRef.current, -10, 40, 20, 20); // Small arrow
      ctx.restore();

      // Draw Frame on top
      ctx.drawImage(frameRef.current, 0, 0, width, height);
    };

    // Load images once
    if (!frameRef.current.src) {
      frameRef.current.src = FrameImage;
      frameRef.current.onload = () => drawAttitudeIndicator();
    }
    if (!arrowRef.current.src) {
      arrowRef.current.src = ArrowImage;
      arrowRef.current.onload = () => drawAttitudeIndicator();
    } else {
      drawAttitudeIndicator();
    }
  }, [roll, pitch]);

  return (
    <div className="AttitudeIndicator">
      <canvas className="AttitudeCanvas" ref={canvasRef} width={300} height={300} />
    </div>
  )
};

export default AttitudeIndicator;
