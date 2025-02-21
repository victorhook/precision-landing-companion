import cv2
import argparse
import numpy as np

def resize_and_add_border(input_path, output_path, size=400, border_size=20):
    # Load the image in grayscale mode
    image = cv2.imread(input_path, cv2.IMREAD_GRAYSCALE)

    if image is None:
        print(f"❌ Error: Could not read {input_path}")
        return

    # Resize the tag to fit inside (size - border) to keep aspect ratio
    inner_size = size - 2 * border_size
    resized_image = cv2.resize(image, (inner_size, inner_size), interpolation=cv2.INTER_NEAREST)

    # Create a white border around the resized image
    bordered_image = cv2.copyMakeBorder(
        resized_image, border_size, border_size, border_size, border_size,
        cv2.BORDER_CONSTANT, value=255  # 255 is white in grayscale
    )

    # Save the final image
    cv2.imwrite(output_path, bordered_image)
    print(f"✅ Resized and bordered image saved as {output_path}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Resize an AprilTag image to 400x400 pixels with a white border.")
    parser.add_argument("input", help="Path to input image")
    parser.add_argument("output", help="Path to save the resized image")

    args = parser.parse_args()
    resize_and_add_border(args.input, args.output)
