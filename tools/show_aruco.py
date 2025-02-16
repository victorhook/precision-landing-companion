import cv2

# Load the uploaded ArUco marker
image = cv2.imread("aruco_marker.png", cv2.IMREAD_GRAYSCALE)

if image is None:
    print("Error: Could not load 'aruco_marker.png'")
    exit()

cv2.imshow("Loaded Marker", image)
cv2.waitKey(0)
cv2.destroyAllWindows()
