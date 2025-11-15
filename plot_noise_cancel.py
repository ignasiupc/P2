import numpy as np
import matplotlib.pyplot as plt
from scipy.io import wavfile

def load_vad(vad_file):
    segments = []
    with open(vad_file, 'r') as f:
        for line in f:
            parts = line.strip().split()
            if len(parts) == 3:
                start, end, label = parts
                segments.append((float(start), float(end), label))
    return segments

def frame_energy(signal, fs, frame_ms=20, hop_ms=10):
    """Retorna l'energia per trama en dB."""
    frame_len = int(frame_ms * 1e-3 * fs)
    hop_len = int(hop_ms * 1e-3 * fs)

    energies = []
    for start in range(0, len(signal) - frame_len + 1, hop_len):
        frame = signal[start:start + frame_len]
        # potència mitjana
        p = np.mean(frame.astype(float) ** 2)
        # passar a dB, evitant log(0)
        p_db = 10 * np.log10(p + 1e-12)
        energies.append(p_db)
    return np.array(energies)

# ==== CONFIGURA AQUI ====
wav_file = "nomsdef1.wav"
vad_file = "nomsdef1.vad"

# === Llegeix àudio ===
fs, audio = wavfile.read(wav_file)

# si és estèreo, fem mitjana a mono
if audio.ndim > 1:
    audio = audio.mean(axis=1)

audio = audio.astype(float)

# === Llegeix VAD ===
vad = load_vad(vad_file)

# Versió "neteja" (silencis posats a 0)
audio_clean = audio.copy()
for start, end, label in vad:
    if label == "S":    # silenci
        i1 = int(start * fs)
        i2 = int(end * fs)
        audio_clean[i1:i2] = 0.0

# === Energia per trama abans i després ===
E1 = frame_energy(audio, fs)        # sense silenciar
E2 = frame_energy(audio_clean, fs)  # silencis cancel·lats

n_trames = np.arange(len(E1))

# === Dibuixar ===
plt.figure(figsize=(9, 5))

plt.grid(True, linestyle="--", alpha=0.4)
plt.plot(n_trames, E1, "o", markersize=3, label="Sense silenciar les trames de silenci (Y1)")
plt.plot(n_trames, E2, "o-", markersize=3, label="Silenciant les trames de silenci (Y2)")

plt.xlabel("Trama (X)")
plt.ylabel("Potència (Y) [dB]")
plt.title("Potència per trama abans i després de silenciar els silencis")
plt.legend()
plt.tight_layout()

plt.savefig("img/potencia_trames_silenci.png", dpi=150)
plt.show()
