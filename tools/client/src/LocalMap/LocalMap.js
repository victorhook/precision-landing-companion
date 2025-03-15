import React, { useRef, useEffect, useState } from "react";
import DroneIcon from "./drone-arrow.svg"; // Drone icon

const LocalMap = ({ x, y, yaw }) => {
  const canvasRef = useRef(null);
  const [trail, setTrail] = useState([]);
  const droneImgRef = useRef(new Image()); // Store drone image

  useEffect(() => {
    // Preload drone image
    if (!droneImgRef.current.src) {
      droneImgRef.current.src = DroneIcon;
    }
  }, []);

  useEffect(() => {
    const canvas = canvasRef.current;
    const ctx = canvas.getContext("2d");
    const width = canvas.width;
    const height = canvas.height;

    const maxTrailTime = 20000; // 20 seconds
    const fadeTime = 5000; // Fade starts after 15 seconds

    // Add new position to the trail
    //setTrail((prevTrail) => {
    //  const newTrail = [...prevTrail, { x, y, timestamp: Date.now() }];
    //  return newTrail.filter((point) => Date.now() - point.timestamp < maxTrailTime);
    //});

    const drawMap = () => {
      ctx.clearRect(0, 0, width, height);
      ctx.beginPath();
      ctx.strokeRect(0, 0, 350, 350);

      // Calculate camera offset (keeps drone near center)
      let cameraX = Math.max(width / 3, Math.min(x, width * 2 / 3));
      let cameraY = Math.max(height / 3, Math.min(y, height * 2 / 3));

      ctx.save();
      ctx.translate(width / 2 - cameraX, height / 2 - cameraY); // Move map to keep drone in view

      // Draw fading trail
      trail.forEach((point) => {
        const age = Date.now() - point.timestamp;
        const opacity = age > fadeTime ? 1 - (age - fadeTime) / (maxTrailTime - fadeTime) : 1;
        ctx.fillStyle = `rgba(0, 255, 0, ${Math.max(opacity, 0)})`;
        ctx.beginPath();
        ctx.arc(point.x, point.y, 3, 0, Math.PI * 2);
        ctx.fill();
      });

      // Draw Drone Icon (Rotated)
      const droneSize = 20;
      ctx.save();
      ctx.translate(x, y); // Move to drone position
      ctx.rotate((yaw * Math.PI) / 180); // Rotate based on yaw
      ctx.drawImage(droneImgRef.current, -droneSize / 2, -droneSize / 2, droneSize, droneSize);
      ctx.restore();

      ctx.restore();
    };

    drawMap();
  }, [x, y, yaw, trail]);

  return (
    <div className="LocalMap text-center">
      <canvas ref={canvasRef} width={350} height={350} />
    </div>
  )
};

export default LocalMap;
