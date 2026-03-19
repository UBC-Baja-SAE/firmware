#include <mcap/writer.hpp>
#include <chrono>
#include <iostream>
#include <string>

// Returns the system time in nanoseconds
mcap::Timestamp now() {
    return mcap::Timestamp(std::chrono::duration_cast<std::chrono::nanoseconds>(
                             std::chrono::system_clock::now().time_since_epoch())
                             .count());
}

// Notice we added the RPM parameter here
int mcapwrite(int currentRpm) {
    mcap::McapWriter writer;

    // 1. Open the file in the dedicated data folder using JSON format
    auto status = writer.open("/home/ubcbaja/logs/output.mcap", mcap::McapWriterOptions("x-jsonschema"));
    if (!status.ok()) {
        std::cerr << "Failed to open MCAP file for writing: " << status.message << "\n";
        return 1; // Error
    }

    // 2. Register a JSON Schema for your engine data
    mcap::Schema rpmSchema("EngineTelemetry", "jsonschema",
        "{\"type\":\"object\",\"properties\":{\"rpm\":{\"type\":\"integer\"}}}");
    writer.addSchema(rpmSchema);

    // 3. Register the Channel (the specific pipe for this data)
    mcap::Channel rpmChannel("engine/rpm", "json", rpmSchema.id);
    writer.addChannel(rpmChannel);

    // 4. Create the JSON payload using the live data passed from Qt
    std::string payload = "{\"rpm\": " + std::to_string(currentRpm) + "}";

    // 5. Pack the message
    mcap::Message msg;
    msg.channelId = rpmChannel.id;
    msg.sequence = 0;
    msg.logTime = now();
    msg.publishTime = msg.logTime;
    msg.data = reinterpret_cast<const std::byte*>(payload.data());
    msg.dataSize = payload.size();

    // 6. Write the message and catch the status to clear the nodiscard warning
    auto writeStatus = writer.write(msg);
    if (!writeStatus.ok()) {
        std::cerr << "Warning: Failed to write MCAP message: " << writeStatus.message << "\n";
    }

    // Finish writing the file and return 0 (Success) to clear the void warning
    writer.close();
    return 0;
}