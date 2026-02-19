import javax.swing.*;
import javax.swing.border.EmptyBorder;
import javax.swing.border.TitledBorder;
import java.awt.*;
import java.awt.event.KeyAdapter;
import java.awt.event.KeyEvent;

public class LuhnGUI extends JFrame {
    private JTextField cardNumberField;
    private JTextArea resultArea;
    private JTextArea processArea;
    private JButton checkButton;
    private JButton clearButton;
    private JButton testValidButton;
    private JButton testInvalidButton;

    public LuhnGUI() {
        setTitle("Алгоритм Луна - Проверка номера карты");
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        setSize(700, 600);
        setLocationRelativeTo(null);

        // Главная панель
        JPanel mainPanel = new JPanel(new BorderLayout(10, 10));
        mainPanel.setBorder(new EmptyBorder(10, 10, 10, 10));

        // Верхняя панель с вводом
        mainPanel.add(createInputPanel(), BorderLayout.NORTH);

        // Центральная панель с результатами
        mainPanel.add(createResultPanel(), BorderLayout.CENTER);

        // Нижняя панель с кнопками
        mainPanel.add(createButtonPanel(), BorderLayout.SOUTH);

        add(mainPanel);

        // Добавляем форматирование ввода
        setupInputFormatting();
    }

    private JPanel createInputPanel() {
        JPanel panel = new JPanel(new BorderLayout(5, 5));
        panel.setBorder(BorderFactory.createTitledBorder(
                BorderFactory.createEtchedBorder(),
                "Введите номер карты",
                TitledBorder.LEFT,
                TitledBorder.TOP,
                new Font("Arial", Font.BOLD, 14)
        ));

        JLabel label = new JLabel("Номер карты:");
        label.setFont(new Font("Arial", Font.PLAIN, 12));

        cardNumberField = new JTextField();
        cardNumberField.setFont(new Font("Monospaced", Font.PLAIN, 18));
        cardNumberField.setPreferredSize(new Dimension(400, 35));

        panel.add(label, BorderLayout.WEST);
        panel.add(cardNumberField, BorderLayout.CENTER);

        return panel;
    }

    private JPanel createResultPanel() {
        JPanel panel = new JPanel(new GridLayout(2, 1, 10, 10));

        // Панель с результатом проверки
        JPanel resultPanel = new JPanel(new BorderLayout());
        resultPanel.setBorder(BorderFactory.createTitledBorder(
                BorderFactory.createEtchedBorder(),
                "Результат проверки",
                TitledBorder.LEFT,
                TitledBorder.TOP,
                new Font("Arial", Font.BOLD, 14)
        ));

        resultArea = new JTextArea();
        resultArea.setEditable(false);
        resultArea.setFont(new Font("Arial", Font.BOLD, 16));
        resultArea.setBackground(new Color(240, 240, 240));
        resultArea.setBorder(new EmptyBorder(10, 10, 10, 10));
        JScrollPane resultScroll = new JScrollPane(resultArea);
        resultScroll.setPreferredSize(new Dimension(650, 80));
        resultPanel.add(resultScroll, BorderLayout.CENTER);

        // Панель с процессом вычисления
        JPanel processPanel = new JPanel(new BorderLayout());
        processPanel.setBorder(BorderFactory.createTitledBorder(
                BorderFactory.createEtchedBorder(),
                "Процесс вычисления",
                TitledBorder.LEFT,
                TitledBorder.TOP,
                new Font("Arial", Font.BOLD, 14)
        ));

        processArea = new JTextArea();
        processArea.setEditable(false);
        processArea.setFont(new Font("Monospaced", Font.PLAIN, 12));
        processArea.setBackground(new Color(250, 250, 250));
        processArea.setBorder(new EmptyBorder(10, 10, 10, 10));
        JScrollPane processScroll = new JScrollPane(processArea);
        processScroll.setPreferredSize(new Dimension(650, 200));
        processPanel.add(processScroll, BorderLayout.CENTER);

        panel.add(resultPanel);
        panel.add(processPanel);

        return panel;
    }

    private JPanel createButtonPanel() {
        JPanel panel = new JPanel(new FlowLayout(FlowLayout.CENTER, 10, 10));

        checkButton = createStyledButton("Проверить", new Color(46, 204, 113));
        clearButton = createStyledButton("Очистить", new Color(52, 73, 94));
        testValidButton = createStyledButton("Тест (корректный)", new Color(52, 152, 219));
        testInvalidButton = createStyledButton("Тест (некорректный)", new Color(231, 76, 60));

        panel.add(checkButton);
        panel.add(clearButton);
        panel.add(testValidButton);
        panel.add(testInvalidButton);

        // Добавляем обработчики
        checkButton.addActionListener(e -> checkNumber());
        clearButton.addActionListener(e -> clearAll());
        testValidButton.addActionListener(e -> testValidNumber());
        testInvalidButton.addActionListener(e -> testInvalidNumber());

        return panel;
    }

    private JButton createStyledButton(String text, Color bgColor) {
        JButton button = new JButton(text);
        button.setFont(new Font("Arial", Font.BOLD, 12));
        button.setBackground(bgColor);
        button.setForeground(Color.WHITE);
        button.setFocusPainted(false);
        button.setBorder(BorderFactory.createRaisedBevelBorder());
        button.setPreferredSize(new Dimension(150, 35));

        // Эффект при наведении
        button.addMouseListener(new java.awt.event.MouseAdapter() {
            public void mouseEntered(java.awt.event.MouseEvent evt) {
                button.setBackground(bgColor.darker());
            }
            public void mouseExited(java.awt.event.MouseEvent evt) {
                button.setBackground(bgColor);
            }
        });

        return button;
    }

    private void setupInputFormatting() {
        cardNumberField.addKeyListener(new KeyAdapter() {
            @Override
            public void keyTyped(KeyEvent e) {
                char c = e.getKeyChar();
                String text = cardNumberField.getText();

                // Разрешаем только цифры и пробел
                if (!Character.isDigit(c) && c != KeyEvent.VK_SPACE && c != KeyEvent.VK_BACK_SPACE) {
                    e.consume();
                }

                // Автоматически добавляем пробел после каждых 4 цифр
                if (Character.isDigit(c) && text.replaceAll("\\s", "").length() % 4 == 0
                        && !text.endsWith(" ")) {
                    cardNumberField.setText(text + " ");
                    cardNumberField.setCaretPosition(cardNumberField.getText().length());
                }
            }
        });
    }

    private void checkNumber() {
        String input = cardNumberField.getText().trim();
        if (input.isEmpty()) {
            JOptionPane.showMessageDialog(this,
                    "Пожалуйста, введите номер карты",
                    "Ошибка",
                    JOptionPane.WARNING_MESSAGE);
            return;
        }

        try {
            boolean isValid = checkLuhnWithDetails(input);
            String cleanNumber = input.replaceAll("\\s", "");

            if (isValid) {
                resultArea.setText("* Номер КОРРЕКТЕН");
                resultArea.setForeground(new Color(39, 174, 96));
            } else {
                resultArea.setText("* Номер НЕКОРРЕКТЕН");
                resultArea.setForeground(new Color(192, 57, 43));
            }

        } catch (IllegalArgumentException ex) {
            JOptionPane.showMessageDialog(this,
                    ex.getMessage(),
                    "Ошибка",
                    JOptionPane.ERROR_MESSAGE);
        }
    }

    private boolean checkLuhnWithDetails(String cardNumber) {
        String cleanNumber = cardNumber.replaceAll("\\s+", "");

        if (!cleanNumber.matches("\\d+")) {
            throw new IllegalArgumentException("Номер должен содержать только цифры и пробелы");
        }

        StringBuilder process = new StringBuilder();
        process.append("=== ПРОЦЕСС ПРОВЕРКИ ===\n");
        process.append("Исходный номер: ").append(cardNumber).append("\n");
        process.append("Номер без пробелов: ").append(cleanNumber).append("\n");
        process.append("Длина номера: ").append(cleanNumber.length()).append(" цифр\n\n");
        process.append("Обработка справа налево:\n");
        process.append("┌─────┬────────────┬──────────┬──────────┐\n");
        process.append("│ Поз │ Цифра      │ Действие │ Результат│\n");
        process.append("├─────┼────────────┼──────────┼──────────┤\n");

        int sum = 0;
        boolean isSecond = false;

        for (int i = cleanNumber.length() - 1; i >= 0; i--) {
            int originalDigit = Character.getNumericValue(cleanNumber.charAt(i));
            int position = cleanNumber.length() - i;
            int processedDigit = originalDigit;
            String action = "×1";

            if (isSecond) {
                processedDigit = originalDigit * 2;
                action = "×2";

                if (processedDigit > 9) {
                    processedDigit -= 9;
                    action += " ( -9)";
                }
            }

            process.append(String.format("│ %3d │ %10d │ %8s │ %8d │\n",
                    position, originalDigit, action, processedDigit));

            sum += processedDigit;
            isSecond = !isSecond;
        }

        process.append("└─────┴────────────┴──────────┴──────────┘\n\n");
        process.append("ИТОГОВАЯ СУММА: ").append(sum).append("\n");
        process.append("Проверка: ").append(sum).append(" % 10 = ").append(sum % 10).append("\n");

        boolean isValid = sum % 10 == 0;
        process.append("РЕЗУЛЬТАТ: ").append(isValid ? "* КОРРЕКТНО" : "* НЕКОРРЕКТНО");

        processArea.setText(process.toString());

        return isValid;
    }

    private void clearAll() {
        cardNumberField.setText("");
        resultArea.setText("");
        processArea.setText("");
    }

    private void testValidNumber() {
        cardNumberField.setText("5062 8212 3456 7892");
        checkNumber();
    }

    private void testInvalidNumber() {
        cardNumberField.setText("5062 8217 3456 7892");
        checkNumber();
    }

    public static void main(String[] args) {
        // Устанавливаем Look and Feel как в системе
        try {
            UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
        } catch (Exception e) {
            e.printStackTrace();
        }

        // Запускаем GUI в потоке обработки событий
        SwingUtilities.invokeLater(() -> {
            new LuhnGUI().setVisible(true);
        });
    }
}