#!/usr/bin/env python3
# Simple script to generate a basic grass block texture for the AzureVoxel project

from PIL import Image
import os
import random

# Create directory if it doesn't exist
os.makedirs("res/textures", exist_ok=True)

# Create a 64x64 pixel image for the grass block
image_size = 64
img = Image.new('RGBA', (image_size, image_size), (0, 0, 0, 0))
pixels = img.load()

# Draw a simple grass block texture
# Top = bright green (grass)
# Sides = brown (dirt)
# Bottom = dark brown (dirt)

# Colors (improved visibility)
grass_green = (85, 180, 55)  # Brighter green for top
dirt_brown = (133, 86, 48)   # Medium brown for sides
dark_brown = (110, 75, 40)   # Slightly darker brown for bottom
outline_color = (50, 50, 50) # Dark gray outline instead of pure black

# Fill the whole image with dirt brown (sides)
for x in range(image_size):
    for y in range(image_size):
        pixels[x, y] = dirt_brown

# Top 1/3 is grass green
for x in range(image_size):
    for y in range(int(image_size / 3)):
        pixels[x, y] = grass_green

# Bottom 1/3 is dark brown
for x in range(image_size):
    for y in range(int(image_size * 2 / 3), image_size):
        pixels[x, y] = dark_brown

# Add some texture variation to make it more interesting
# Add grass "strands" to the top
random.seed(42)  # For reproducible results

for x in range(image_size):
    for y in range(int(image_size / 3)):
        # Add some darker green pixels randomly
        if random.random() < 0.2:
            darker = (int(grass_green[0] * 0.8), int(grass_green[1] * 0.8), int(grass_green[2] * 0.8))
            pixels[x, y] = darker
        # Add some lighter green pixels randomly
        elif random.random() < 0.1:
            lighter = (min(255, int(grass_green[0] * 1.2)), min(255, int(grass_green[1] * 1.2)), min(255, int(grass_green[2] * 1.2)))
            pixels[x, y] = lighter

# Add some texture to the dirt
for x in range(image_size):
    # Middle section (sides)
    for y in range(int(image_size / 3), int(image_size * 2 / 3)):
        if random.random() < 0.1:
            darker = (int(dirt_brown[0] * 0.9), int(dirt_brown[1] * 0.9), int(dirt_brown[2] * 0.9))
            pixels[x, y] = darker
        elif random.random() < 0.05:
            lighter = (min(255, int(dirt_brown[0] * 1.1)), min(255, int(dirt_brown[1] * 1.1)), min(255, int(dirt_brown[2] * 1.1)))
            pixels[x, y] = lighter
    
    # Bottom section
    for y in range(int(image_size * 2 / 3), image_size):
        if random.random() < 0.1:
            darker = (int(dark_brown[0] * 0.9), int(dark_brown[1] * 0.9), int(dark_brown[2] * 0.9))
            pixels[x, y] = darker
        elif random.random() < 0.05:
            lighter = (min(255, int(dark_brown[0] * 1.1)), min(255, int(dark_brown[1] * 1.1)), min(255, int(dark_brown[2] * 1.1)))
            pixels[x, y] = lighter

# Draw a darker outline around the edges
for x in range(image_size):
    pixels[x, 0] = outline_color
    pixels[x, image_size-1] = outline_color
    pixels[x, int(image_size/3)-1] = outline_color
    pixels[x, int(image_size*2/3)-1] = outline_color

for y in range(image_size):
    pixels[0, y] = outline_color
    pixels[image_size-1, y] = outline_color

# Save the image
texture_path = "res/textures/grass_block.png"
img.save(texture_path)
print(f"Created texture at {texture_path}")

# Also copy to build directory if it exists
build_texture_path = "build/res/textures/grass_block.png"
if os.path.exists("build"):
    os.makedirs("build/res/textures", exist_ok=True)
    img.save(build_texture_path)
    print(f"Copied texture to {build_texture_path}")

print("Done!") 