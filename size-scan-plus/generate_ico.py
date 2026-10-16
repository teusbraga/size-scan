import struct
import math

def generate_sizescan_ico(filename="sizescan.ico"):
    sizes = [16, 32, 48, 64, 128]
    images_data = []

    # Cores (B, G, R, A)
    BG_DARK = (42, 40, 38, 255)         # #26282a
    BORDER_COLOR = (67, 64, 61, 255)    # #3d4043
    MINT_GREEN = (181, 201, 119, 255)   # #77c9b5
    MINT_BRIGHT = (215, 235, 170, 255)  # #aae7d7
    AMBER = (100, 169, 226, 255)        # #e2a964
    CORAL = (107, 107, 217, 255)        # #d96b6b
    BLUE_MUTED = (214, 163, 107, 255)   # #6ba3d6
    WHITE = (245, 242, 240, 255)        # #f0f2f5
    TRANS = (0, 0, 0, 0)

    for sz in sizes:
        pixels = [[TRANS for _ in range(sz)] for _ in range(sz)]
        cx, cy = sz / 2.0, sz / 2.0
        r_outer = sz * 0.46
        r_inner = sz * 0.22

        for y in range(sz):
            for x in range(sz):
                dx = x - cx + 0.5
                dy = y - cy + 0.5
                dist = math.sqrt(dx*dx + dy*dy)

                if dist <= r_outer:
                    # Borda do disco
                    if dist >= r_outer - (sz * 0.08):
                        pixels[y][x] = MINT_GREEN
                    elif dist >= r_inner:
                        # Fatias de gráfico/disco escaneado
                        angle = math.atan2(dy, dx) # -pi a pi
                        if angle < 0:
                            angle += 2 * math.pi
                        deg = math.degrees(angle)

                        if deg < 110:
                            pixels[y][x] = MINT_GREEN
                        elif deg < 190:
                            pixels[y][x] = AMBER
                        elif deg < 260:
                            pixels[y][x] = CORAL
                        elif deg < 320:
                            pixels[y][x] = BLUE_MUTED
                        else:
                            pixels[y][x] = MINT_BRIGHT
                    elif dist > r_inner - (sz * 0.06):
                        # Borda do anel interno
                        pixels[y][x] = BORDER_COLOR
                    else:
                        # Furo interno do disco
                        pixels[y][x] = BG_DARK
                else:
                    pixels[y][x] = TRANS

        # Desenhar uma lupa / feixe de scanner moderno sobreposto
        # Eixo da lente da lupa (canto inferior direito)
        lens_cx = cx + sz * 0.08
        lens_cy = cy - sz * 0.08
        lens_r = sz * 0.28

        for y in range(sz):
            for x in range(sz):
                dx = x - lens_cx + 0.5
                dy = y - lens_cy + 0.5
                dist = math.sqrt(dx*dx + dy*dy)
                
                # Aro da lente do scanner
                if abs(dist - lens_r) <= (sz * 0.065):
                    pixels[y][x] = WHITE
                elif dist < lens_r:
                    # Brilho sutil dentro da lente
                    cur = pixels[y][x]
                    if cur[3] > 0:
                        # Mistura com verde menta brilhante
                        pixels[y][x] = (
                            min(255, int(cur[0] * 0.6 + MINT_BRIGHT[0] * 0.4)),
                            min(255, int(cur[1] * 0.6 + MINT_BRIGHT[1] * 0.4)),
                            min(255, int(cur[2] * 0.6 + MINT_BRIGHT[2] * 0.4)),
                            255
                        )

        # Cabo da lupa no canto inferior direito
        handle_len = sz * 0.22
        handle_w = max(1.0, sz * 0.08)
        for step in range(int(sz * 0.25), int(sz * 0.48)):
            hx = lens_cx + step * 0.7071
            hy = lens_cy + step * 0.7071
            for ox in range(-int(handle_w), int(handle_w) + 1):
                for oy in range(-int(handle_w), int(handle_w) + 1):
                    px = int(hx + ox)
                    py = int(hy + oy)
                    if 0 <= px < sz and 0 <= py < sz:
                        pixels[py][px] = MINT_GREEN

        # Converter para BGRA no formato DIB (linhas de baixo para cima)
        raw_dib = bytearray()
        for y in reversed(range(sz)):
            for x in range(sz):
                b, g, r, a = pixels[y][x]
                raw_dib.extend([b, g, r, a])

        # Máscara AND (1 bit por pixel, alinhada em múltiplos de 32 bits por linha)
        row_bytes = (sz + 31) // 32 * 4
        and_mask = bytearray(row_bytes * sz)
        for y_idx, y in enumerate(reversed(range(sz))):
            for x in range(sz):
                if pixels[y][x][3] == 0:
                    byte_pos = y_idx * row_bytes + (x // 8)
                    bit_pos = 7 - (x % 8)
                    and_mask[byte_pos] |= (1 << bit_pos)

        # BITMAPINFOHEADER (40 bytes)
        # biHeight = 2 * sz (XOR height + AND height)
        bmi = struct.pack("<IIIHHIIIIII",
            40,         # biSize
            sz,         # biWidth
            sz * 2,     # biHeight
            1,          # biPlanes
            32,         # biBitCount
            0,          # biCompression (BI_RGB)
            len(raw_dib) + len(and_mask), # biSizeImage
            0, 0, 0, 0
        )

        img_bytes = bmi + bytes(raw_dib) + bytes(and_mask)
        images_data.append((sz, img_bytes))

    # Construir arquivo .ICO
    # ICONDIR (6 bytes): idReserved(2)=0, idType(2)=1, idCount(2)=len
    ico_data = bytearray()
    ico_data.extend(struct.pack("<HHH", 0, 1, len(images_data)))

    # ICONDIRENTRY (16 bytes cada)
    offset = 6 + 16 * len(images_data)
    for sz, img_bytes in images_data:
        w_b = sz if sz < 256 else 0
        h_b = sz if sz < 256 else 0
        ico_data.extend(struct.pack("<BBBBHHII",
            w_b, h_b, 0, 0, 1, 32, len(img_bytes), offset
        ))
        offset += len(img_bytes)

    for sz, img_bytes in images_data:
        ico_data.extend(img_bytes)

    with open(filename, "wb") as f:
        f.write(ico_data)

    print(f"Icone gerado com sucesso: {filename} ({len(ico_data)} bytes)")

if __name__ == "__main__":
    generate_sizescan_ico("src/sizescan.ico")
