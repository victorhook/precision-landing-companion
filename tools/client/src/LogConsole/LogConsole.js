import React, { useState, useEffect, useMemo, useRef } from "react";
import "./LogConsole.css";

const LOG_LEVELS = [
  { label: "DEBUG", value: 0 },
  { label: "INFO", value: 1 },
  { label: "WARNING", value: 2 },
  { label: "ERROR", value: 3 }
];

const LOG_GROUPS = { "ALL": "ALL", 0: "MAIN", 1: "ArduPilot" };

const LEVEL_COLORS = {
  1: "#00ff00", // INFO - Green
  2: "#ffff00", // WARNING - Yellow
  3: "#ff0000", // ERROR - Red
  0: "#AAAAAA", // DEBUG - Gray
  DEFAULT: "#ffffff" // Default - White
};

const LogConsole = ({ logs, setLogs }) => {
  const logContainerRef = useRef(null);
  const [isUserScrolling, setIsUserScrolling] = useState(false);
  const [filterLevel, setFilterLevel] = useState(0);
  const [filterGroup, setFilterGroup] = useState("ALL");

  const formattedLogs = useMemo(() => {
    return logs
      .filter((log) => log.level >= filterLevel)
      .filter((log) => filterGroup === "ALL" || LOG_GROUPS[log.group] === filterGroup)
      .map((log) => {
        const date = new Date(log.timestamp);
        const time = date.toLocaleTimeString("en-US", { hour12: false }) + ":" + String(date.getMilliseconds()).padStart(3, "0");
        return {
          time,
          msg: log.msg,
          level: log.level,
          group: LOG_GROUPS[log.group] || "Unknown",
        };
      });
  }, [logs, filterLevel, filterGroup]);

  useEffect(() => {
    const logContainer = logContainerRef.current;
    if (!logContainer) return;

    const handleScroll = () => {
      const isAtBottom = logContainer.scrollHeight - logContainer.scrollTop <= logContainer.clientHeight + 10;
      setIsUserScrolling(!isAtBottom);
    };

    logContainer.addEventListener("scroll", handleScroll);
    return () => logContainer.removeEventListener("scroll", handleScroll);
  }, []);

  useEffect(() => {
    if (!isUserScrolling && logContainerRef.current) {
      logContainerRef.current.scrollTop = logContainerRef.current.scrollHeight;
    }
  }, [formattedLogs, isUserScrolling]);

  const handleClear = () => {
    setLogs([]);
  };

  return (
    <div className="LogConsoleWrapper">
      <div className="LogConsoleControls">
        <select value={filterLevel} onChange={(e) => setFilterLevel(Number(e.target.value))}>
          {LOG_LEVELS.map(({ label, value }) => (
            <option key={value} value={value}>{label}</option>
          ))}
        </select>
        <select value={filterGroup} onChange={(e) => setFilterGroup(e.target.value)}>
          {Object.entries(LOG_GROUPS).map(([key, group]) => (
            <option key={key} value={group}>{group}</option>
          ))}
        </select>
        <button onClick={handleClear}>Clear</button>
      </div>
      <div className="LogConsole" ref={logContainerRef}>
        {formattedLogs.map((log, index) => (
          <p key={index} className="log-entry" style={{ color: LEVEL_COLORS[log.level] || LEVEL_COLORS.DEFAULT }}>
            [{log.time}] ({log.group}) {log.msg}
          </p>
        ))}
      </div>
    </div>
  );
};

export default LogConsole;
