# Komposition Knack-Reel: 1080x1920, 30 fps, GUI-Frames + Captions + Cursor + Endkarte
# Aufruf (im Ordner mit out/ aus ReelRender): python3 compose.py [preview t1 t2 ...]  ->  knack_reel.mp4
import json, subprocess, sys
from PIL import Image, ImageDraw, ImageFont, ImageFilter
import numpy as np

W, H, FPS = 1080, 1920, 30
D = json.load(open("out/events.json"))
TOTAL = D["total"]; GUI_UNTIL = D["guiUntil"]; R0 = D["rects"]; CUR = D["cursor"]
FONT = "/usr/share/fonts/opentype/inter/"
def font(w, s): return ImageFont.truetype(FONT + {"black": "Inter-Black.otf", "bold": "Inter-Bold.otf", "med": "Inter-Medium.otf", "semi": "Inter-SemiBold.otf"}[w], s)

BG1, BG2 = (22, 17, 18), (13, 11, 12)
TEXT, DIM = (240, 236, 236), (136, 128, 128)
RED, ORANGE, CYAN = (255, 70, 70), (255, 146, 60), (90, 200, 230)
URL = "github.com/danikoenig/Knack/releases"
gui = R0["gui"]
def pad(r, p): return [r[0] - p, r[1] - p, r[2] + 2 * p, r[3] + 2 * p]
FOCUS = {
    "full": [0, 0, gui[2], gui[3]],
    "calib": [0, 1150, gui[2], 340],
    "shape": [0, 150, gui[2], 880],
}
SCENES = [  # start, ende, titel, untertitel, fokus, farbe
    (0.0, 2.2, "", "", "full", RED),
    (2.2, 6.2, "CALIBRATE", "plays your signal, sets the input for you", "calib", ORANGE),
    (6.2, 16.5, "ONE KNOB", "turn it up  ·  watch the wave get squashed", "full", RED),
    (16.5, 21.5, "SOFT TO HARD", "from warm saturation to hard clipping", "shape", RED),
    (21.5, GUI_UNTIL, "MIX", "parallel clipping, keep your punch", "full", CYAN),
]
def scene_at(t):
    for i, s in enumerate(SCENES):
        if s[0] <= t < s[1]: return i
    return len(SCENES) - 1
def ease(x): x = max(0.0, min(1.0, x)); return x * x * (3 - 2 * x)

BOX_W, BOX_TOP, BOX_MAXH = 1020, 560, 1180
def crop_at(t):
    i = scene_at(t); cur = FOCUS[SCENES[i][4]]
    if i == 0: return cur
    prev = FOCUS[SCENES[i - 1][4]]
    u = ease((t - SCENES[i][0]) / 0.5)
    return [prev[k] + (cur[k] - prev[k]) * u for k in range(4)]
def box_for(crop):
    s = BOX_W / crop[2]; h = crop[3] * s
    if h > BOX_MAXH: s = BOX_MAXH / crop[3]; h = BOX_MAXH
    w = crop[2] * s
    return (W - w) / 2, BOX_TOP + (BOX_MAXH - h) * 0.3, w, h, s

def make_bg():
    a = np.zeros((H, W, 3), dtype="float32")
    u = np.linspace(0, 1, H)[:, None]
    for k in range(3): a[:, :, k] = BG1[k] + (BG2[k] - BG1[k]) * u
    g = Image.new("RGB", (W, H)); d = ImageDraw.Draw(g)
    d.ellipse([W / 2 - 560, 800 - 560, W / 2 + 560, 800 + 560], fill=(70, 16, 16))
    g = np.asarray(g.filter(ImageFilter.GaussianBlur(200)), dtype="float32")
    return Image.fromarray(np.clip(a + g * 0.55, 0, 255).astype("uint8"))
BG = make_bg()

def centered(d, y, text, f, fill):
    w = d.textlength(text, font=f); d.text(((W - w) / 2, y), text, font=f, fill=fill)

def cursor_at(f):
    if f >= len(CUR) or CUR[f] is None or isinstance(CUR[f], type(None)): return None
    c = CUR[f]
    return (c[0], c[1], bool(c[2])) if isinstance(c, list) else None
press = [f for f in range(1, len(CUR)) if cursor_at(f) and cursor_at(f)[2] and not (cursor_at(f - 1) and cursor_at(f - 1)[2])]

def draw_cursor(img, x, y, pressed, f):
    d = ImageDraw.Draw(img, "RGBA")
    for pf in press:
        dt = (f - pf) / FPS
        if 0 <= dt < 0.35:
            r = 14 + dt * 150; a = int(200 * (1 - dt / 0.35))
            d.ellipse([x - r, y - r, x + r, y + r], outline=(255, 255, 255, a), width=4)
    if pressed: d.ellipse([x - 20, y - 20, x + 20, y + 20], fill=(255, 255, 255, 60))
    s = 1.9; pts = [(0, 0), (0, 22), (5.5, 17), (9.5, 26), (13, 24.5), (9, 16), (16, 16)]
    poly = [(x + px * s, y + py * s) for px, py in pts]
    d.polygon([(px + 2, py + 3) for px, py in poly], fill=(0, 0, 0, 110))
    d.polygon(poly, fill=(255, 255, 255, 255), outline=(20, 20, 24, 255))
    d.line(poly + [poly[0]], fill=(20, 20, 24, 255), width=3)

def gui_frame(f): return Image.open(f"out/frames/{f:04d}.jpg").convert("RGB")
F_CAP_T, F_CAP_S, F_SMALL = font("black", 96), font("med", 40), font("semi", 34)

def render(f):
    t = f / FPS
    img = BG.copy(); d = ImageDraw.Draw(img, "RGBA")
    if t < GUI_UNTIL + 0.6:
        gf = gui_frame(min(f, int(GUI_UNTIL * FPS) - 1))
        crop = crop_at(t); bx, by, bw, bh, s = box_for(crop)
        region = gf.crop((int(crop[0]), int(crop[1]), int(crop[0] + crop[2]), int(crop[1] + crop[3]))).resize((int(bw), int(bh)), Image.LANCZOS)
        i = scene_at(t)
        if i == 0: region = Image.blend(region, Image.new("RGB", region.size, BG2), 0.5)
        sh = Image.new("RGBA", (W, H), (0, 0, 0, 0))
        ImageDraw.Draw(sh).rounded_rectangle([bx + 6, by + 18, bx + bw + 6, by + bh + 24], radius=26, fill=(0, 0, 0, 160))
        shb = sh.filter(ImageFilter.GaussianBlur(22)); img.paste(shb, (0, 0), shb)
        mask = Image.new("L", region.size, 0)
        ImageDraw.Draw(mask).rounded_rectangle([0, 0, region.size[0] - 1, region.size[1] - 1], radius=22, fill=255)
        img.paste(region, (int(bx), int(by)), mask)
        d = ImageDraw.Draw(img, "RGBA")
        d.rounded_rectangle([bx, by, bx + bw, by + bh], radius=22, outline=(255, 255, 255, 30), width=2)
        sc = SCENES[i]
        if i == 0:
            u = ease(t / 0.5)
            centered(d, 210 - 30 * (1 - u), "KNACK", font("black", 190), TEXT)
            centered(d, 430, "one knob clipper", font("semi", 46), DIM)
        else:
            u = ease((t - sc[0]) / 0.35); a = int(255 * u); yo = 26 * (1 - u)
            d.rounded_rectangle([W / 2 - 40, 270 + yo, W / 2 + 40, 278 + yo], radius=4, fill=sc[5] + (a,))
            centered(d, 305 + yo, sc[2], F_CAP_T, TEXT + (a,))
            centered(d, 430 + yo, sc[3], F_CAP_S, DIM + (a,))
            d.text((72, 108), "KNACK", font=font("black", 42), fill=TEXT)
        c = cursor_at(f)
        if c and i > 0:
            cx = bx + (c[0] - crop[0]) * s; cy = by + (c[1] - crop[1]) * s
            if by - 40 < cy < by + bh + 40: draw_cursor(img, cx, cy, c[2], f)
        d = ImageDraw.Draw(img, "RGBA")
        centered(d, 1810, "free  ·  clipper  ·  VST3 + AU + CLAP  ·  Mac", F_SMALL, DIM)
    if t >= GUI_UNTIL:
        u = ease((t - GUI_UNTIL) / 0.6)
        end = BG.copy(); e = ImageDraw.Draw(end, "RGBA")
        centered(e, 520, "KNACK", font("black", 170), TEXT)
        centered(e, 740, "one knob clipper", font("semi", 46), DIM)
        for k, ln in enumerate(["soft to hard clipping", "auto input with Calibrate", "limiter  ·  mix  ·  HQ oversampling"]):
            centered(e, 880 + k * 64, ln, font("semi", 40), TEXT)
        centered(e, 1140, "VST3  ·  AU  ·  CLAP  ·  Standalone", font("bold", 36), RED)
        fu = font("bold", 38); tw = e.textlength(URL, font=fu); bx0 = (W - tw) / 2 - 36
        e.rounded_rectangle([bx0, 1240, W - bx0, 1340], radius=24, fill=(30, 22, 22, 255), outline=RED + (255,), width=3)
        centered(e, 1266, URL, fu, TEXT)
        centered(e, 1380, "free download  ·  link in bio", font("med", 36), DIM)
        centered(e, 1700, "by Dani Koenig", font("semi", 36), DIM)
        img = Image.blend(img, end, u) if u < 1 else end
    return img

if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "preview":
        for ts in sys.argv[2:]: render(int(float(ts) * FPS)).save(f"/tmp/prev_{float(ts):05.2f}.jpg", quality=88)
        sys.exit(0)
    n = int(round(TOTAL * FPS))
    p = subprocess.Popen(["ffmpeg", "-y", "-loglevel", "error", "-f", "rawvideo", "-pix_fmt", "rgb24", "-s", f"{W}x{H}", "-r", str(FPS),
                          "-i", "-", "-c:v", "libx264", "-preset", "veryfast", "-crf", "18", "-pix_fmt", "yuv420p", "video_tmp.mp4"], stdin=subprocess.PIPE)
    for f in range(n):
        p.stdin.write(render(f).tobytes())
        if f % 150 == 0: print("compose", f, "/", n, flush=True)
    p.stdin.close(); p.wait()
    subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-i", "video_tmp.mp4", "-i", "out/audio.wav", "-c:v", "copy",
                    "-c:a", "aac", "-b:a", "256k", "-shortest", "-movflags", "+faststart", "knack_reel.mp4"], check=True)
    print("video done")
