import React, { useEffect, useRef } from "react";
import "./SmallDataTable.css";

const SmallDataTable = ({ data }) => {
    const tableRef = useRef(null);

    useEffect(() => {
        if (tableRef.current) {
            tableRef.current.scrollTop = tableRef.current.scrollHeight;
        }
    }, [data]); // ✅ Auto-scroll when new data arrives

    return (
        <div className="small-data-table-container">
            <div className="small-data-table" ref={tableRef}>
                <table>
                    <tbody>
                        {Object.entries(data).map(([key, value], index) => (
                            <tr key={index}>
                                <td className="key">{key}</td>
                                <td className="value">
                                  {typeof value === "number" ? value.toFixed(3) : value}
                                </td>
                            </tr>
                        ))}
                    </tbody>
                </table>
            </div>
        </div>
    );
};

export default SmallDataTable;
