import cv2
import mediapipe as mp
import numpy as np
import tkinter as tk
from tkinter import filedialog, Label, Button, Frame, messagebox
from PIL import Image, ImageTk
import os
import sys

# Отключаем GPU для предотвращения ошибок X Server
os.environ['OPENCV_IO_ENABLE_OPENEXR'] = '0'
os.environ['TF_CPP_MIN_LOG_LEVEL'] = '2'  # Уменьшаем логи TensorFlow


class FaceDetectorApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Детектор областей лица")
        self.root.geometry("1000x800")

        # Инициализация MediaPipe
        self.init_mediapipe()

        # Индексы для областей лица
        self.LEFT_EYE = [33, 133, 157, 158, 159, 160, 161, 173]
        self.RIGHT_EYE = [362, 263, 387, 386, 385, 384, 398, 466]
        self.LIPS = [61, 291, 0, 17, 405, 314]
        self.FACE_OVAL = [10, 338, 297, 332, 284, 251, 389, 356, 454, 323, 361, 288,
                          397, 365, 379, 378, 400, 377, 152, 148, 176, 149, 150,
                          136, 172, 58, 132, 93, 234, 127, 162, 21, 54, 103, 67, 109]

        self.create_widgets()
        self.original_image = None
        self.processed_image = None

    def init_mediapipe(self):
        """Инициализация MediaPipe"""
        try:
            self.mp_face_mesh = mp.solutions.face_mesh
            self.face_mesh = self.mp_face_mesh.FaceMesh(
                static_image_mode=True,
                max_num_faces=10,
                refine_landmarks=True,
                min_detection_confidence=0.5
            )
            print("MediaPipe инициализирован успешно")
        except Exception as e:
            print(f"Ошибка инициализации MediaPipe: {e}")
            messagebox.showerror("Ошибка", f"Не удалось инициализировать MediaPipe: {e}")
            sys.exit(1)

    def create_widgets(self):
        """Создание интерфейса"""
        # Верхняя панель
        top_frame = Frame(self.root)
        top_frame.pack(pady=10)

        self.btn_load = Button(top_frame, text="Загрузить изображение",
                               command=self.load_image,
                               bg="#4CAF50", fg="white", padx=20, pady=5)
        self.btn_load.pack(side=tk.LEFT, padx=5)

        self.btn_detect = Button(top_frame, text="Детектировать области",
                                 command=self.detect_regions,
                                 bg="#2196F3", fg="white", padx=20, pady=5,
                                 state="disabled")
        self.btn_detect.pack(side=tk.LEFT, padx=5)

        self.btn_save = Button(top_frame, text="Сохранить результат",
                               command=self.save_result,
                               bg="#FF9800", fg="white", padx=20, pady=5,
                               state="disabled")
        self.btn_save.pack(side=tk.LEFT, padx=5)

        # Информационная метка
        self.info_label = Label(self.root, text="Загрузите изображение")
        self.info_label.pack(pady=5)

        # Область для изображения
        self.image_label = Label(self.root, bg="gray")
        self.image_label.pack(pady=10, padx=10, fill=tk.BOTH, expand=True)

        # Статус бар
        self.status_label = Label(self.root, text="Готов", bd=1, relief=tk.SUNKEN, anchor=tk.W)
        self.status_label.pack(side=tk.BOTTOM, fill=tk.X)

    def load_image(self):
        """Загрузка изображения"""
        file_path = filedialog.askopenfilename(
            filetypes=[("Images", "*.jpg *.jpeg *.png *.bmp")]
        )
        if file_path:
            try:
                self.original_image = cv2.imread(file_path)
                self.original_image = cv2.cvtColor(self.original_image, cv2.COLOR_BGR2RGB)
                self.display_image(self.original_image)
                self.btn_detect.config(state="normal")
                self.info_label.config(text=f"Загружено: {os.path.basename(file_path)}")
            except Exception as e:
                messagebox.showerror("Ошибка", f"Не удалось загрузить: {e}")

    def display_image(self, image):
        """Отображение изображения"""
        if image is None:
            return

        # Конвертация для Tkinter
        pil_image = Image.fromarray(image)

        # Масштабирование под окно
        max_size = (800, 600)
        pil_image.thumbnail(max_size, Image.Resampling.LANCZOS)

        photo = ImageTk.PhotoImage(pil_image)
        self.image_label.config(image=photo)
        self.image_label.image = photo

    def get_bbox(self, landmarks, indices, shape):
        """Получение bounding box"""
        h, w = shape[:2]
        points = []
        for idx in indices:
            if idx < len(landmarks.landmark):
                x = int(landmarks.landmark[idx].x * w)
                y = int(landmarks.landmark[idx].y * h)
                points.append((x, y))

        if points:
            x_coords = [p[0] for p in points]
            y_coords = [p[1] for p in points]
            padding = 5
            x1 = max(0, min(x_coords) - padding)
            y1 = max(0, min(y_coords) - padding)
            x2 = min(w, max(x_coords) + padding)
            y2 = min(h, max(y_coords) + padding)
            return (x1, y1, x2, y2)
        return None

    def detect_regions(self):
        """Детекция областей"""
        if self.original_image is None:
            return

        try:
            image = self.original_image.copy()
            rgb_image = cv2.cvtColor(image, cv2.COLOR_RGB2BGR)
            results = self.face_mesh.process(rgb_image)

            if results.multi_face_landmarks:
                faces_count = len(results.multi_face_landmarks)
                self.info_label.config(text=f"Найдено лиц: {faces_count}")

                for face_landmarks in results.multi_face_landmarks:
                    # Лицо
                    face_bbox = self.get_bbox(face_landmarks, self.FACE_OVAL, image.shape)
                    if face_bbox:
                        x1, y1, x2, y2 = face_bbox
                        cv2.rectangle(image, (x1, y1), (x2, y2), (255, 0, 0), 2)
                        cv2.putText(image, "FACE", (x1, y1 - 5),
                                    cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 0, 0), 1)

                    # Левый глаз
                    left_bbox = self.get_bbox(face_landmarks, self.LEFT_EYE, image.shape)
                    if left_bbox:
                        x1, y1, x2, y2 = left_bbox
                        cv2.rectangle(image, (x1, y1), (x2, y2), (0, 255, 0), 2)
                        cv2.putText(image, "LEFT EYE", (x1, y1 - 5),
                                    cv2.FONT_HERSHEY_SIMPLEX, 0.4, (0, 255, 0), 1)

                    # Правый глаз
                    right_bbox = self.get_bbox(face_landmarks, self.RIGHT_EYE, image.shape)
                    if right_bbox:
                        x1, y1, x2, y2 = right_bbox
                        cv2.rectangle(image, (x1, y1), (x2, y2), (0, 255, 0), 2)
                        cv2.putText(image, "RIGHT EYE", (x1, y1 - 5),
                                    cv2.FONT_HERSHEY_SIMPLEX, 0.4, (0, 255, 0), 1)

                    # Губы
                    lips_bbox = self.get_bbox(face_landmarks, self.LIPS, image.shape)
                    if lips_bbox:
                        x1, y1, x2, y2 = lips_bbox
                        cv2.rectangle(image, (x1, y1), (x2, y2), (0, 0, 255), 2)
                        cv2.putText(image, "LIPS", (x1, y1 - 5),
                                    cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 255), 1)

                self.processed_image = image
                self.display_image(image)
                self.btn_save.config(state="normal")
                self.status_label.config(text="Детекция завершена")
            else:
                messagebox.showinfo("Информация", "Лица не найдены")

        except Exception as e:
            messagebox.showerror("Ошибка", f"Ошибка детекции: {e}")

    def save_result(self):
        """Сохранение результата"""
        if self.processed_image is None:
            return

        file_path = filedialog.asksaveasfilename(
            defaultextension=".jpg",
            filetypes=[("JPEG", "*.jpg"), ("PNG", "*.png")]
        )

        if file_path:
            save_image = cv2.cvtColor(self.processed_image, cv2.COLOR_RGB2BGR)
            cv2.imwrite(file_path, save_image)
            messagebox.showinfo("Успех", "Изображение сохранено")

    def __del__(self):
        if hasattr(self, 'face_mesh'):
            self.face_mesh.close()


if __name__ == "__main__":
    root = tk.Tk()
    app = FaceDetectorApp(root)
    root.mainloop()