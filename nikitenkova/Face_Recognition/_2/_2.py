import os
import cv2
import face_recognition
import numpy as np
from pathlib import Path
import tkinter as tk
from tkinter import filedialog, messagebox, Label, Button, Frame
from PIL import Image, ImageTk


class FaceRecognitionApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Распознавание лиц")
        self.root.geometry("1200x800")  # Немного увеличил высоту

        # База данных лиц
        self.known_face_encodings = []
        self.known_face_names = []

        self.create_widgets()

    def create_widgets(self):
        # Верхняя панель
        top_frame = Frame(self.root, height=50)
        top_frame.pack(fill=tk.X, padx=10, pady=10)

        Button(top_frame, text="Загрузить шаблоны",
               command=self.load_templates).pack(side=tk.LEFT, padx=5)

        Button(top_frame, text="Тестовое изображение",
               command=self.load_test_image).pack(side=tk.LEFT, padx=5)

        Button(top_frame, text="Распознать",
               command=self.recognize_faces).pack(side=tk.LEFT, padx=5)

        Button(top_frame, text="Сохранить результат",
               command=self.save_result).pack(side=tk.LEFT, padx=5)

        # Статус
        self.status_label = Label(self.root, text="Готов к работе")
        self.status_label.pack(pady=5)

        # Область с изображениями
        image_frame = Frame(self.root)
        image_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)

        # Левое изображение
        left_frame = Frame(image_frame, relief=tk.SUNKEN, borderwidth=2)
        left_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=5)

        Label(left_frame, text="Оригинал").pack()
        self.original_label = Label(left_frame, bg="gray")
        self.original_label.pack(fill=tk.BOTH, expand=True)

        # Правое изображение
        right_frame = Frame(image_frame, relief=tk.SUNKEN, borderwidth=2)
        right_frame.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True, padx=5)

        Label(right_frame, text="Результат").pack()
        self.result_label = Label(right_frame, bg="gray")
        self.result_label.pack(fill=tk.BOTH, expand=True)

        # ===== УЛУЧШЕНИЕ №3: Список распознанных лиц =====
        # Панель с результатами под изображениями
        results_frame = Frame(self.root, relief=tk.SUNKEN, borderwidth=1)
        results_frame.pack(fill=tk.X, padx=10, pady=5)

        Label(results_frame, text="Распознанные лица:",
              font=("Arial", 10, "bold")).pack(anchor=tk.W, padx=5, pady=2)

        self.results_label = Label(results_frame, text="",
                                   justify=tk.LEFT, anchor=tk.W,
                                   font=("Arial", 9))
        self.results_label.pack(fill=tk.X, padx=15, pady=2)
        # ===== КОНЕЦ УЛУЧШЕНИЯ =====

        # Переменные
        self.original_image = None
        self.result_image = None
        self.original_path = None
        self.face_names = []
        self.face_confidences = []

    def load_templates(self):
        folder = filedialog.askdirectory()
        if not folder:
            return

        self.status_label.config(text="Загрузка шаблонов...")
        self.root.update()

        self.known_face_encodings = []
        self.known_face_names = []

        for file in Path(folder).glob('*'):
            if file.suffix.lower() in ['.jpg', '.jpeg', '.png']:
                try:
                    image = face_recognition.load_image_file(str(file))
                    encodings = face_recognition.face_encodings(image)
                    if encodings:
                        self.known_face_encodings.append(encodings[0])
                        name = file.stem.replace('_', ' ').title()
                        self.known_face_names.append(name)
                except:
                    pass

        self.status_label.config(text=f"Загружено {len(self.known_face_names)} лиц")
        messagebox.showinfo("Успех", f"Загружено {len(self.known_face_names)} лиц")

    def load_test_image(self):
        file_path = filedialog.askopenfilename(
            filetypes=[("Images", "*.jpg *.jpeg *.png")]
        )

        if file_path:
            self.original_path = file_path
            self.original_image = face_recognition.load_image_file(file_path)
            self.display_image(self.original_image, self.original_label)
            self.status_label.config(text=f"Загружено: {Path(file_path).name}")

            # Очищаем предыдущие результаты
            self.results_label.config(text="")
            self.result_label.config(image="")

    def display_image(self, image, label, max_size=(500, 400)):
        if image is None:
            return

        pil_image = Image.fromarray(image)
        pil_image.thumbnail(max_size, Image.Resampling.LANCZOS)
        photo = ImageTk.PhotoImage(pil_image)

        label.config(image=photo)
        label.image = photo

    def recognize_faces(self):
        if self.original_image is None:
            messagebox.showwarning("Предупреждение", "Загрузите тестовое изображение")
            return

        if not self.known_face_encodings:
            messagebox.showwarning("Предупреждение", "Загрузите шаблоны")
            return

        self.status_label.config(text="Распознавание...")
        self.root.update()

        image = self.original_image.copy()
        face_locations = face_recognition.face_locations(image)
        face_encodings = face_recognition.face_encodings(image, face_locations)

        # Очищаем списки
        self.face_names = []
        self.face_confidences = []

        # Конвертируем для OpenCV
        image_cv = cv2.cvtColor(image, cv2.COLOR_RGB2BGR)

        for (top, right, bottom, left), face_encoding in zip(face_locations, face_encodings):
            distances = face_recognition.face_distance(self.known_face_encodings, face_encoding)
            best_match = np.argmin(distances)

            if distances[best_match] < 0.6:
                name = self.known_face_names[best_match]
                confidence = 1 - distances[best_match]
                self.face_names.append(name)
                self.face_confidences.append(confidence)
            else:
                name = "Unknown"
                self.face_names.append(name)
                self.face_confidences.append(0)

            cv2.rectangle(image_cv, (left, top), (right, bottom), (0, 255, 0), 2)
            cv2.putText(image_cv, name, (left, top - 5),
                        cv2.FONT_HERSHEY_SIMPLEX, 2, (0, 255, 0), 2)

        self.result_image = cv2.cvtColor(image_cv, cv2.COLOR_BGR2RGB)
        self.display_image(self.result_image, self.result_label)

        # ===== УЛУЧШЕНИЕ №3: Формируем текст для отображения =====
        results_text = ""
        if face_locations:
            for i, (name, conf) in enumerate(zip(self.face_names, self.face_confidences)):
                if conf > 0:
                    results_text += f"  Лицо {i + 1}: {name} (уверенность: {conf:.2f})\n"
                else:
                    results_text += f"  Лицо {i + 1}: {name}\n"
        else:
            results_text = "  Лица не найдены"

        self.results_label.config(text=results_text)
        # ===== КОНЕЦ УЛУЧШЕНИЯ =====

        self.status_label.config(text=f"Распознано {len(face_locations)} лиц")

    def save_result(self):
        if self.result_image is None:
            messagebox.showwarning("Предупреждение", "Нет результата для сохранения")
            return

        file_path = filedialog.asksaveasfilename(
            defaultextension=".jpg",
            filetypes=[("JPEG", "*.jpg"), ("PNG", "*.png")]
        )

        if file_path:
            save_image = cv2.cvtColor(self.result_image, cv2.COLOR_RGB2BGR)
            cv2.imwrite(file_path, save_image)
            messagebox.showinfo("Успех", "Результат сохранен")


def main():
    root = tk.Tk()
    app = FaceRecognitionApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()