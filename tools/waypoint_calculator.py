import tkinter as tk
from tkinter import messagebox
import math
import os

def hesapla_ve_kaydet():
    try:
        # Arayüzden verileri al
        origin_lat = float(entry_lat.get())
        origin_lon = float(entry_lon.get())
        heading_deg = float(entry_heading.get()) 
        
        # Orijin hücresi (2,2) sarı matın merkezi
        origin_grid_x = 2
        origin_grid_y = 2
        
        # Şartname değerleri
        CELL_SIZE = 1.25 # metre
        R_EARTH = 6378137.0 # Dünya yarıçapı (metre)
        
        # (2,2) merkezinin yerel metrik konumu
        origin_abs_x = (origin_grid_x - 1) * CELL_SIZE + (CELL_SIZE / 2.0)
        origin_abs_y = (origin_grid_y - 1) * CELL_SIZE + (CELL_SIZE / 2.0)
        
        heading_rad = math.radians(heading_deg)
        origin_lat_rad = math.radians(origin_lat)
        
        # --- YOL GÜNCELLEMESİ BURADA ---
        # Görseldeki dosya yoluna göre ayarlandı: ~/Desktop/kitt_kodelig/Waypoint Calculator
        ev_dizini = os.path.expanduser("~")
        masaustu = os.path.join(ev_dizini, "Desktop")
        if not os.path.exists(masaustu):
            masaustu = os.path.join(ev_dizini, "Masaüstü")
            
        hedef_klasor = os.path.join(masaustu, "kitt_kodelig", "Waypoint Calculator")
        
        # Klasör herhangi bir sebeple silinirse diye otomatik oluşturma güvencesi
        if not os.path.exists(hedef_klasor):
            os.makedirs(hedef_klasor)
            
        dosya_yolu = os.path.join(hedef_klasor, "kodelig_wgs84_haritasi.txt")
        # -------------------------------
        
        with open(dosya_yolu, "w", encoding="utf-8") as file:
            file.write(f"Orijin (2,2) GPS: {origin_lat:.7f}, {origin_lon:.7f}\n")
            file.write(f"Saha Yönelimi (Sapma): {heading_deg} Derece\n")
            file.write("-" * 60 + "\n")
            file.write("Grid (X,Y) | Enlem (Latitude) | Boylam (Longitude)\n")
            file.write("-" * 60 + "\n")
            
            for y in range(9):
                for x in range(9):
                    # Köşenin mutlak yerel konumu
                    corner_abs_x = x * CELL_SIZE
                    corner_abs_y = y * CELL_SIZE
                    
                    # (2,2) merkezine göre yerel mesafe (metre)
                    local_dx = corner_abs_x - origin_abs_x # Sağ
                    local_dy = corner_abs_y - origin_abs_y # İleri
                    
                    # Rotasyon Matrisi
                    dN = (local_dy * math.cos(heading_rad)) - (local_dx * math.sin(heading_rad))
                    dE = (local_dy * math.sin(heading_rad)) + (local_dx * math.cos(heading_rad))
                    
                    # Metreyi Enlem/Boylam derecesine çevirme
                    delta_lat = (dN / R_EARTH) * (180.0 / math.pi)
                    delta_lon = (dE / (R_EARTH * math.cos(origin_lat_rad))) * (180.0 / math.pi)
                    
                    # Yeni GPS koordinatları
                    target_lat = origin_lat + delta_lat
                    target_lon = origin_lon + delta_lon
                    
                    file.write(f"Kese ({x},{y})  | {target_lat:.7f}    | {target_lon:.7f}\n")
                    
        messagebox.showinfo("Başarılı", f"GPS koordinatları hesaplandı!\n\nDosya: {dosya_yolu}")
        
    except ValueError:
        messagebox.showerror("Hata", "Lütfen koordinatları ve açıyı sayısal olarak doğru formatta girin (Örn: 40.7654321).")

# --- Arayüz (GUI) Tasarımı ---
root = tk.Tk()
root.title("KODELİG Mission Planner Waypoint Üretici")
root.geometry("450x300")
root.resizable(False, False)

tk.Label(root, text="(2,2) Sarı Mat Merkezi GPS Verisi", font=("Arial", 12, "bold")).pack(pady=10)

# Enlem (Latitude) Girişi
frame_lat = tk.Frame(root)
frame_lat.pack(pady=5)
tk.Label(frame_lat, text="Enlem (Latitude): ", width=20, anchor="e").pack(side=tk.LEFT)
entry_lat = tk.Entry(frame_lat, width=20)
entry_lat.pack(side=tk.LEFT)

# Boylam (Longitude) Girişi
frame_lon = tk.Frame(root)
frame_lon.pack(pady=5)
tk.Label(frame_lon, text="Boylam (Longitude): ", width=20, anchor="e").pack(side=tk.LEFT)
entry_lon = tk.Entry(frame_lon, width=20)
entry_lon.pack(side=tk.LEFT)

# Sapma Açısı Girişi
frame_heading = tk.Frame(root)
frame_heading.pack(pady=5)
tk.Label(frame_heading, text="Gerçek Kuzey Sapması (°): ", width=20, anchor="e").pack(side=tk.LEFT)
entry_heading = tk.Entry(frame_heading, width=20)
entry_heading.insert(0, "0.0") # Varsayılan tam kuzey
entry_heading.pack(side=tk.LEFT)

# Çalıştırma Butonu
btn = tk.Button(root, text="WGS84 Koordinatlarını Üret", command=hesapla_ve_kaydet, bg="#0052cc", fg="white", font=("Arial", 10, "bold"))
btn.pack(pady=20)

root.mainloop()
