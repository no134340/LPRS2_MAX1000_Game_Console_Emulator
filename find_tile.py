#!/usr/bin/env python3

from PIL import Image
import glob
import os
import sys


def make_tiles_array(main_array, w, h):
    tile_imgs = []
    cnt = 0
    for y in range(0, h, 16):
        for x in range(0, w, 16):
            tile_img = Image.new('RGB', (16, 16))
            tile_array = tile_img.load()
            for i in range(16):
                for j in range(16):
                    tile_array[i, j] = main_array[i + x, y + j]
            tile_imgs.append(tile_img)
            # tile_img.save(f"tile{cnt}.png")
            # cnt += 1
    return tile_imgs


io = sys.stderr
# make the header file
fp = open("screen_tile_index.h", "w+")
fp.write("#ifndef SCREEN_TILE_INDEX_H\n" +
         "#define SCREEN_TILE_INDEX_H\n" +
         "#include <stdint.h>\n\n")
fp.write("extern uint16_t tile_num_x;\n")
fp.write("extern uint16_t tile_num_y;\n\n")

# make the source file
fp = open("screen_tile_index.c", "w+")
fp.write('#include "screen_tile_index.h"\n\n')
fp.write("uint16_t tile_num_x = 18;\n")
fp.write("uint16_t tile_num_y = 8;\n\n")
fp.close()


# make the tile images array
tiles = Image.open("../images/tiles.png").convert('RGB')
tiles_array = tiles.load()

tile_imgs = make_tiles_array(tiles_array, tiles.size[0], tiles.size[1])

# get every map path
img_names = []
os.chdir(os.getcwd() + "/../Maps")
for file in glob.glob("*.png"):
    img_names.append(file)

img_names = sorted(img_names)
# for every map, generate index matrix

io.write("Generating lookup index matrices...\n")

for file in img_names:

    io.write("{0}...".format(file))

    map_img = Image.open(file)
    rgb_img = map_img.convert('RGB')
    map_array = rgb_img.load()
    width, height = map_img.size[0], map_img.size[1]

    indices = []

    for y in range(0, height - 8, 16):
        for x in range(0, width, 16):
            ind = -1

            for idx, tile in enumerate(tile_imgs):
                ta = tile.load()
                match = 0
                for i in range(16):
                    for j in range(16):
                        if ta[i, j] == map_array[x + i, y + j]:
                            match += 1
                if match > 16*16 - 10:
                    ind = idx
                    break
            if ind == -1:
                indices.append(144)
            else:
                indices.append(ind)

    # print(indices)
    #
    # print(len(indices))

    os.chdir(os.getcwd() + "/../build")

    fp = open("screen_tile_index.h", "a+")
    fp.write("extern uint8_t {0}__p[];\n".format(file.split('/')[-1].rstrip('.png')))
    fp.close()

    fp = open("screen_tile_index.c", "a+")
    fp.write("uint8_t {0}__p[] = ".format(file.split('/')[-1].rstrip('.png')) + "{\n")
    pr_str = ""
    for i, ind in enumerate(indices):
        pr_str += ("{0},".format(ind))
        if i % 16 == 15 and i != 0:
            pr_str += ("\n")
    pr_str.rstrip(",")
    pr_str += "};\n\n"
    fp.write(pr_str)
    fp.close()

    os.chdir(os.getcwd() + "/../Maps")

    io.write("Done\n")

os.chdir(os.getcwd() + "/../build")
fp = open("screen_tile_index.h", "a+")
fp.write("\n#endif //SCREEN_TILE_INDEX_H ")
fp.close()
