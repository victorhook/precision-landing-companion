//TELEMETRY_IP = '192.168.4.1'
//TELEMETRY_IP = '192.168.0.202'
const TELEMETRY_IP = '127.0.0.1'
const TELEMETRY_PORT = 8080

class Api {

    constructor(baseUrl = `http://${TELEMETRY_IP}:${TELEMETRY_PORT}`) {
        this.baseUrl = baseUrl;
    }

    async #getJson(endpoint) {
        try {
            const response = await fetch(`${this.baseUrl}/${endpoint}`);
            const data = await response.json();
            return data;
        } catch (error) {
            console.error("Failed to fetch telemetry:", error);
            return null;
        }
    }

    async #postJson(endpoint, body) {
        try {
            const response = await fetch(`${this.baseUrl}/${endpoint}`, {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: JSON.stringify(body),
            });
        
            if (!response.ok) throw new Error("Failed to update parameters");
        
            return await response.json();
        } catch (error) {
            console.error("Error sending parameters:", error);
        }

        return null;
    }

    /** ✅ Fetch telemetry data */
    async getTelemetry() {
        return this.#getJson('telemetry');
    }

    /** ✅ Send detection parameters */
    async setDetectionParams(params) {
        this.#postJson('set_detection_params', params);
    }

    /** Send command */
    async setCommand(command) {
        return this.#postJson('command', command);
    }
}

export default new Api();
