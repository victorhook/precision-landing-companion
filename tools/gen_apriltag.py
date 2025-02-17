import cv2
import numpy as np

DICT = cv2.aruco.DICT_APRILTAG_16H5

# Load the correct ArUco dictionary
dictionary = cv2.aruco.getPredefinedDictionary(DICT)

# Set marker properties
marker_id = 1
marker_size = 400  # Marker area (without border)
border_size = 400   # White border thickness
total_size = marker_size + 2 * border_size  # Final image size

FILENAME = f'apriltag_{marker_id}.jpg'

# Generate marker
marker_image = cv2.aruco.generateImageMarker(dictionary, marker_id, marker_size)

# Create a white background
bordered_image = np.ones((total_size, total_size), dtype=np.uint8) * 255  # All white

# Place the marker in the center
bordered_image[border_size:border_size + marker_size, border_size:border_size + marker_size] = marker_image

# Save and display the new marker
cv2.imwrite(FILENAME, bordered_image)
print(f"✅ New marker with border saved as '{FILENAME}'")
cv2.imshow("ArUco Marker with Border", bordered_image)
cv2.waitKey(0)
cv2.destroyAllWindows()
