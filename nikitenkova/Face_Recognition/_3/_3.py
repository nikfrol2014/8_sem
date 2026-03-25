import os
import cv2
import numpy as np
from PIL import Image, ImageTk, ImageDraw, ImageFont
import tkinter as tk
from tkinter import filedialog, messagebox, Label, Button, Frame
from pathlib import Path
import threading
import math

# Отключаем X Server ошибки
os.environ['OPENCV_IO_ENABLE_OPENEXR'] = '0'
os.environ['LIBGL_ALWAYS_SOFTWARE'] = '1'


class SimpleEmotionGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Определитель эмоций")
        self.root.geometry("900x650")
        self.root.configure(bg='#f0f0f0')

        # Словарь эмоций
        self.emotions_ru = {
            'happy': 'Счастье', 'sad': 'Грусть', 'neutral': 'Нейтрально',
            'surprise': 'Удивление', 'angry': 'Злость', 'fear': 'Страх'
        }

        # Загружаем каскады OpenCV
        cascade_path = cv2.data.haarcascades + 'haarcascade_frontalface_default.xml'
        self.face_cascade = cv2.CascadeClassifier(cascade_path)
        self.eye_cascade = cv2.CascadeClassifier(cv2.data.haarcascades + 'haarcascade_eye.xml')
        self.smile_cascade = cv2.CascadeClassifier(cv2.data.haarcascades + 'haarcascade_smile.xml')
        
        # Пытаемся загрузить русский шрифт
        self.font = None
        font_paths = [
            "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf",
            "/System/Library/Fonts/Helvetica.ttc",  # для macOS
            "C:/Windows/Fonts/arial.ttf"  # для Windows
        ]
        
        for path in font_paths:
            if os.path.exists(path):
                try:
                    self.font = ImageFont.truetype(path, 24)
                    self.font_small = ImageFont.truetype(path, 18)
                    break
                except:
                    continue
        
        if self.font is None:
            # Если шрифт не найден, используем стандартный
            self.font = ImageFont.load_default()
            self.font_small = ImageFont.load_default()

        self.create_widgets()
        self.image = None
        self.result_image = None

    def create_widgets(self):
        # Верхняя панель
        top_frame = Frame(self.root, bg='#f0f0f0')
        top_frame.pack(pady=10)

        Button(top_frame, text="Загрузить фото", command=self.load_image,
               font=("Arial", 11), width=15, height=1).pack(side=tk.LEFT, padx=5)

        Button(top_frame, text="Определить эмоции", command=self.detect_emotion,
               font=("Arial", 11), width=15, height=1).pack(side=tk.LEFT, padx=5)

        # Статус
        self.status = Label(self.root, text="Загрузите фотографию", 
                           font=("Arial", 10), bg='#f0f0f0', fg='#666')
        self.status.pack(pady=5)

        # Изображение
        image_frame = Frame(self.root, bg='white', relief=tk.SUNKEN, bd=2)
        image_frame.pack(pady=10, padx=10, fill=tk.BOTH, expand=True)
        
        self.image_label = Label(image_frame, bg='white')
        self.image_label.pack(expand=True, fill=tk.BOTH)

        # Результат
        self.result_label = Label(self.root, text="", font=("Arial", 12), 
                                  bg='#f0f0f0', fg='#333', justify=tk.LEFT)
        self.result_label.pack(pady=10)

    def load_image(self):
        file_path = filedialog.askopenfilename(
            filetypes=[("Images", "*.jpg *.jpeg *.png *.bmp")]
        )

        if file_path:
            try:
                self.image = cv2.imread(file_path)
                if self.image is None:
                    raise ValueError("Не удалось загрузить изображение")
                    
                self.image = cv2.cvtColor(self.image, cv2.COLOR_BGR2RGB)
                self.show_image(self.image)
                self.status.config(text=f"Загружено: {Path(file_path).name}")
                self.result_label.config(text="")
            except Exception as e:
                messagebox.showerror("Ошибка", f"Не удалось загрузить изображение")

    def show_image(self, image):
        h, w = image.shape[:2]
        max_h, max_w = 450, 600
        
        if h > max_h or w > max_w:
            scale = min(max_h / h, max_w / w)
            new_h, new_w = int(h * scale), int(w * scale)
            image = cv2.resize(image, (new_w, new_h))

        pil_image = Image.fromarray(image)
        photo = ImageTk.PhotoImage(pil_image)

        self.image_label.config(image=photo)
        self.image_label.image = photo

    def add_text_to_image(self, image, text, position, color=(0, 255, 0)):
        """Добавление русского текста на изображение с помощью PIL"""
        # Конвертируем OpenCV image в PIL
        pil_image = Image.fromarray(image)
        draw = ImageDraw.Draw(pil_image)
        
        # Выбираем шрифт
        if len(text) > 20:
            font = self.font_small
        else:
            font = self.font
        
        # Добавляем текст
        draw.text(position, text, font=font, fill=color)
        
        # Конвертируем обратно в OpenCV формат
        return np.array(pil_image)

    def detect_emotion(self):
        if self.image is None:
            messagebox.showwarning("", "Сначала загрузите фото")
            return

        self.status.config(text="Анализ...")
        self.result_label.config(text="Анализирую...")
        self.root.update()

        thread = threading.Thread(target=self._analyze)
        thread.daemon = True
        thread.start()

    def analyze_face_features(self, gray_face, face_roi):
        """Улучшенный анализ черт лица"""
        
        # 1. Анализ улыбки
        smiles = self.smile_cascade.detectMultiScale(gray_face, 1.8, 20)
        has_smile = len(smiles) > 0
        
        # 2. Анализ глаз
        eyes = self.eye_cascade.detectMultiScale(gray_face, 1.1, 10)
        eyes_count = len(eyes)
        
        # 3. Анализ формы рта
        height, width = gray_face.shape
        mouth_y = int(height * 0.7)
        mouth_region = gray_face[mouth_y:height, :]
        
        grad_x = cv2.Sobel(mouth_region, cv2.CV_64F, 1, 0, ksize=3)
        grad_x = np.abs(grad_x)
        mouth_intensity = np.mean(grad_x)
        
        # 4. Анализ бровей
        brow_y = int(height * 0.3)
        brow_region = gray_face[0:brow_y, :]
        brow_intensity = np.mean(brow_region)
        
        # 5. Общая яркость
        mean_bright = np.mean(gray_face)
        std_bright = np.std(gray_face)
        
        # Вычисляем вероятности эмоций
        scores = {}
        
        # Счастье
        if has_smile:
            scores['happy'] = 75 + min(20, mouth_intensity / 10)
        else:
            scores['happy'] = 30
            
        # Удивление
        if eyes_count >= 2 and std_bright > 40:
            scores['surprise'] = 70
        else:
            scores['surprise'] = 20
            
        # Грусть
        if mouth_intensity < 15 and mean_bright < 120:
            scores['sad'] = 70
        else:
            scores['sad'] = 25
            
        # Злость
        if brow_intensity < 80 and mean_bright < 100:
            scores['angry'] = 65
        else:
            scores['angry'] = 20
            
        # Страх
        if eyes_count >= 2 and std_bright > 50 and mouth_intensity > 20:
            scores['fear'] = 60
        else:
            scores['fear'] = 15
            
        # Нейтрально
        if max(scores.values()) < 50:
            scores['neutral'] = 50
        else:
            scores['neutral'] = 30
        
        # Нормализуем
        total = sum(scores.values())
        if total > 0:
            for k in scores:
                scores[k] = (scores[k] / total) * 100
        
        dominant = max(scores, key=scores.get)
        confidence = scores[dominant]
        
        return dominant, confidence, scores

    def _analyze(self):
        try:
            gray = cv2.cvtColor(self.image, cv2.COLOR_RGB2GRAY)
            faces = self.face_cascade.detectMultiScale(gray, 1.1, 5, minSize=(60, 60))
            
            if len(faces) == 0:
                self.root.after(0, lambda: messagebox.showinfo("", 
                              "Лицо не обнаружено\nПопробуйте другое фото"))
                self.root.after(0, lambda: self.status.config(text="Готов"))
                return
            
            result_image = self.image.copy()
            results_text = ""
            
            for i, (x, y, w, h) in enumerate(faces):
                # Вырезаем лицо
                face_roi = self.image[y:y+h, x:x+w]
                gray_face = gray[y:y+h, x:x+w]
                
                # Анализируем эмоции
                emotion, confidence, scores = self.analyze_face_features(gray_face, face_roi)
                ru_emotion = self.emotions_ru.get(emotion, emotion)
                
                # Выбираем цвет рамки (в формате RGB для OpenCV)
                if emotion == 'happy':
                    color = (0, 255, 0)  # зеленый
                elif emotion == 'sad':
                    color = (255, 0, 0)  # синий
                elif emotion == 'angry':
                    color = (0, 0, 255)  # красный
                elif emotion == 'surprise':
                    color = (0, 255, 255)  # желтый
                else:
                    color = (255, 255, 0)  # голубой
                
                # Рисуем рамку
                cv2.rectangle(result_image, (x, y), (x+w, y+h), color, 2)
                
                # Добавляем текст с помощью PIL (чтобы был русский)
                text = f"{ru_emotion} ({confidence:.0f}%)"
                # Позиция текста (над рамкой)
                text_position = (x, y - 25 if y - 25 > 0 else y + 5)
                # Цвет для текста (такой же как рамка)
                text_color = tuple(reversed(color))  # OpenCV BGR -> RGB для PIL
                
                result_image = self.add_text_to_image(result_image, text, text_position, text_color)
                
                # Формируем текст результата
                if len(faces) > 1:
                    results_text += f"Лицо {i+1}: {ru_emotion} - {confidence:.0f}%\n"
                else:
                    results_text += f"Основная эмоция: {ru_emotion}\n"
                    results_text += f"Уверенность: {confidence:.0f}%\n\n"
                    results_text += "Детализация:\n"
                    
                    sorted_scores = sorted(scores.items(), key=lambda x: x[1], reverse=True)
                    for em, score in sorted_scores[:4]:
                        ru = self.emotions_ru.get(em, em)
                        results_text += f"  {ru}: {score:.0f}%\n"
            
            if len(faces) > 1:
                results_text = f"Найдено лиц: {len(faces)}\n\n{results_text}"
            
            self.root.after(0, self._update_result, result_image, results_text)
            
        except Exception as e:
            self.root.after(0, lambda: messagebox.showerror("Ошибка", str(e)))
        finally:
            self.root.after(0, lambda: self.status.config(text="Готов"))

    def _update_result(self, image, text):
        self.show_image(image)
        self.result_label.config(text=text)


if __name__ == "__main__":
    root = tk.Tk()
    app = SimpleEmotionGUI(root)
    root.mainloop()