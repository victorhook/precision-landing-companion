import React, { useState, useEffect } from "react";
import {
  LineChart,
  Line,
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip,
  Legend,
  ResponsiveContainer,
} from "recharts";

const RealTimeGraph = ({ newData, timeWindow = 30 }) => {
  const [data, setData] = useState([]);

  useEffect(() => {
    if (!newData) return;

    setData((prevData) => {
      const now = Date.now();
      const updatedData = [...prevData, { time: now, ...newData }].slice(-100);

      // Keep only last X seconds of data
      return updatedData.filter((d) => now - d.time <= timeWindow * 1000);
    });
  }, [newData, timeWindow]);

  return (
    <ResponsiveContainer width="100%" height={300}>
      <LineChart data={data}>
        <CartesianGrid strokeDasharray="3 3" />
        <XAxis
          dataKey="time"
          tickFormatter={(time) => new Date(time).toLocaleTimeString()}
        />
        <YAxis />
        <Tooltip />
        <Legend />
        {Object.keys(newData).map((key, index) => (
          <Line key={key} type="monotone" dataKey={key} stroke={`hsl(${index * 100}, 80%, 50%)`} />
        ))}
      </LineChart>
    </ResponsiveContainer>
  );
};

export default RealTimeGraph;
