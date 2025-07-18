import struct
import numpy as np
import matplotlib.pyplot as plt
import os

# Directory containing the .bin files
data_dir = "/Users/yeabayalew/Documents/research/data/"
file_pattern = "nirs_"
start_index = 0 # Starting file index (e.g., nirs_00003.bin)
end_index = 5 # Ending file index (adjust based on your files)

all_frames = []
channels = None
samples_per_frame = None

# Aggregate data from multiple files
for i in range(start_index, end_index + 1):
    file_path = os.path.join(data_dir, f"{file_pattern}{i:05d}.bin")
    if os.path.exists(file_path):
        print(f"Processing file: {file_path}")
        with open(file_path, 'rb') as f:
            # Read header (5 integers)
            header = struct.unpack('5i', f.read(5 * 4))
            version, curr_channels, curr_samples_per_frame, frame_size, block_size = header
            print(f"Header: {header}")

            # Validate consistency across files
            if channels is None:
                channels = curr_channels
                samples_per_frame = curr_samples_per_frame
            elif channels != curr_channels or samples_per_frame != curr_samples_per_frame:
                print(f"Warning: Inconsistent header in {file_path}. Skipping.")
                continue

            # Read frames
            while True:
                frame_data = f.read(frame_size)
                if not frame_data:
                    break
                if len(frame_data) == frame_size:
                    frame = struct.unpack(f'{channels}d', frame_data[:channels * 8])  # 8 bytes per double
                    all_frames.append(frame)
    else:
        print(f"File not found: {file_path}")

# Convert to NumPy array
if all_frames:
    data = np.array(all_frames)
    print(f"Total frames read: {len(all_frames)}")

    # Plot only channel 0
    plt.figure(figsize=(12, 6))
    plt.plot(data[:, 0], label="Channel 0")
    plt.title("Channel 0 Over Time")
    plt.xlabel("Frame Index")
    plt.ylabel("Voltage")
    plt.legend()
    plt.grid(True)
    plt.show()
else:
    print("No data frames were read from the files.")
