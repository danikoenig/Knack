try:
    from lib import install_text
except ImportError:
    pass

CONTENT = {
"title": "KNACK", "file": "Knack", "repo": "Knack",
"accent": "#ff8a3d", "accent2": "#f0485a",
"en": {
 "subtitle": "Clipper",
 "kind": "Audio effect for macOS (Apple Silicon)  \xb7  VST3, Audio Unit, CLAP and Standalone",
 "by": "by Dani Koenig",
 "intro": ("Knack is a simple clipper with one big knob. Turn CLIP up and the signal is pushed harder into the clipper. "
           "The curve at the top shows what happens to the wave, from a clean sine to a fully squashed one, and the plugin glows red the more you turn. "
           "Use it on kicks, drums, buses or the master to get loudness and punch without a complicated setup."),
 "quick_title": "Quick start",
 "quick": [
   ("1. Calibrate", "Press <b>CALIBRATE</b> and play the loudest part of your track for about 3 seconds. Knack sets INPUT so the loudest peak lands at -6 dBFS."),
   ("2. Clip", "Turn the big <b>CLIP</b> knob up until you hear the punch you want. The curve and the red glow show how hard you push."),
   ("3. Shape", "Move the <b>SOFT / HARD</b> slider: left is round and smooth, right is a hard clip."),
   ("4. Output", "Pull <b>OUTPUT</b> down to start the limiter and keep the level under control."),
 ],
 "first": "Tip: with CLIP at 20 % and SOFT / HARD in the middle you already hear a gentle glue. Then add MIX below 100 % for parallel clipping.",
 "sections": [
  ("#ff7a2f", "CLIPPER", [
    ("CLIP", "The big knob: how hard the signal is pushed into the clipper, 0 to 100 % (starts at 20 %). The plugin glows red as you turn up."),
    ("SOFT / HARD", "Slider above the knob. Left: round soft clip. Right: hard clip."),
    ("Curve display", "Shows the wave: at 0 % a normal sine, at 100 % fully squashed. It follows CLIP and SOFT / HARD."),
    ("MIX", "Blends the clipped and the dry signal for parallel (New York style) clipping. 100 % = fully clipped. Latency is compensated."),
  ]),
  ("#1fb3c8", "INPUT", [
    ("INPUT", "Level into the clipper, -24 to +24 dB, with a level meter next to it."),
    ("CALIBRATE", "Plays your signal for about 3 seconds and sets INPUT so the loudest peak lands at -6 dBFS."),
  ]),
  ("#f0485a", "OUTPUT", [
    ("OUTPUT", "Slider with limiter. 0 = 0 dBFS, so the limiter does not work at the start. Pull it down to start the limiter, down to -12 dB."),
    ("GR", "Gain reduction of the limiter in dB, in steps of 0.1 dB."),
    ("LEVEL", "Output level in dB."),
    ("HQ", "4x oversampling for cleaner clipping. It adds a little latency, which Knack reports to your DAW."),
  ]),
 ],
 "recipes": [
   ("Punchy kick", "CLIP 30 to 50 %, SOFT / HARD a bit to the right, MIX 100 %. Check the sound with HQ on."),
   ("Loud drum bus", "CLIP 25 %, SOFT / HARD in the middle, MIX 50 to 70 % for parallel clipping, OUTPUT -1 dB."),
   ("Master glue", "CLIP 10 to 15 %, SOFT, MIX 100 %, OUTPUT to taste. Use CALIBRATE first."),
   ("Hard edge", "CLIP 60 % or more with HARD for aggressive clipping on bass or 808s. Turn OUTPUT down to protect your ears."),
 ],
 "tips": [
   "Use CALIBRATE with the loudest part of your music, not with silence.",
   "All parameters can be automated in the DAW.",
   "HQ is on by default. Switch it off only if you need the lowest latency.",
   "Knack sits well after a compressor and before the final limiter.",
 ],
 "install": install_text("en", "Knack", "Knack", "Knack"),
},
"de": {
 "subtitle": "Clipper",
 "kind": "Audio-Effekt für macOS (Apple Silicon)  \xb7  VST3, Audio Unit, CLAP und Standalone",
 "by": "von Dani Koenig",
 "intro": ("Knack ist ein einfacher Clipper mit einem grossen Regler. Drehst du CLIP auf, wird das Signal stärker in den Clipper gedrückt. "
           "Die Kurve oben zeigt, was mit der Welle passiert, von einem sauberen Sinus bis zu einer völlig gequetschten Welle, und das Plugin leuchtet röter, je weiter du drehst. "
           "Nimm ihn auf Kicks, Drums, Bussen oder dem Master für Lautheit und Punch ohne komplizierten Aufbau."),
 "quick_title": "Schnellstart",
 "quick": [
   ("1. Kalibrieren", "<b>CALIBRATE</b> drücken und die lauteste Stelle deines Tracks etwa 3 Sekunden spielen. Knack stellt INPUT so ein, dass die lauteste Spitze bei -6 dBFS landet."),
   ("2. Clippen", "Den grossen <b>CLIP</b>-Regler aufdrehen, bis du den gewünschten Punch hörst. Kurve und rotes Leuchten zeigen, wie stark du drückst."),
   ("3. Formen", "Den Schieberegler <b>SOFT / HARD</b> bewegen: links rund und weich, rechts harter Clip."),
   ("4. Output", "<b>OUTPUT</b> nach unten ziehen, um den Limiter zu starten und den Pegel im Griff zu halten."),
 ],
 "first": "Tipp: Mit CLIP auf 20 % und SOFT / HARD in der Mitte hörst du schon einen sanften Kleber. Danach MIX unter 100 % für paralleles Clippen.",
 "sections": [
  ("#ff7a2f", "CLIPPER", [
    ("CLIP", "Der grosse Regler: wie stark das Signal in den Clipper gedrückt wird, 0 bis 100 % (Start bei 20 %). Das Plugin leuchtet röter, je weiter du aufdrehst."),
    ("SOFT / HARD", "Schieberegler über dem Regler. Links: runder Soft-Clip. Rechts: harter Clip."),
    ("Kurvenanzeige", "Zeigt die Welle: bei 0 % ein normaler Sinus, bei 100 % völlig gequetscht. Sie folgt CLIP und SOFT / HARD."),
    ("MIX", "Mischt das geclippte und das trockene Signal für paralleles Clippen (New-York-Stil). 100 % = ganz geclippt. Die Latenz wird ausgeglichen."),
  ]),
  ("#1fb3c8", "INPUT", [
    ("INPUT", "Pegel in den Clipper, -24 bis +24 dB, mit Pegelanzeige daneben."),
    ("CALIBRATE", "Spielt dein Signal etwa 3 Sekunden und stellt INPUT so ein, dass die lauteste Spitze bei -6 dBFS landet."),
  ]),
  ("#f0485a", "OUTPUT", [
    ("OUTPUT", "Schieberegler mit Limiter. 0 = 0 dBFS, der Limiter arbeitet am Anfang also nicht. Nach unten ziehen startet den Limiter, bis -12 dB."),
    ("GR", "Gain Reduction des Limiters in dB, in 0.1-dB-Schritten."),
    ("LEVEL", "Ausgangspegel in dB."),
    ("HQ", "4-faches Oversampling für saubereres Clippen. Es bringt etwas Latenz, die Knack der DAW meldet."),
  ]),
 ],
 "recipes": [
   ("Punchige Kick", "CLIP 30 bis 50 %, SOFT / HARD etwas nach rechts, MIX 100 %. Den Sound mit HQ an prüfen."),
   ("Laute Drum-Gruppe", "CLIP 25 %, SOFT / HARD in der Mitte, MIX 50 bis 70 % für paralleles Clippen, OUTPUT -1 dB."),
   ("Master-Kleber", "CLIP 10 bis 15 %, SOFT, MIX 100 %, OUTPUT nach Gehör. Vorher CALIBRATE benutzen."),
   ("Harte Kante", "CLIP 60 % oder mehr mit HARD für aggressives Clippen auf Bass oder 808. OUTPUT herunterdrehen, schont die Ohren."),
 ],
 "tips": [
   "CALIBRATE mit der lautesten Stelle deiner Musik benutzen, nicht mit Stille.",
   "Alle Parameter lassen sich in der DAW automatisieren.",
   "HQ ist standardmässig an. Nur ausschalten, wenn du die geringste Latenz brauchst.",
   "Knack passt gut nach einem Kompressor und vor den letzten Limiter.",
 ],
 "install": install_text("de", "Knack", "Knack", "Knack"),
},
}
