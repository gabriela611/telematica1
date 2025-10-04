import javax.swing.*;
import javax.swing.border.EmptyBorder;
import javax.swing.border.LineBorder;
import java.awt.*;
import java.io.*;
import java.net.*;
import java.time.LocalTime;
import java.time.format.DateTimeFormatter;
import java.util.HashMap;

public class AdminClient extends JFrame {
    // networking
    private Socket socket;
    private BufferedReader in;
    private BufferedWriter out;
    private volatile boolean receiving = false;
    private Thread recvThread;

    // ui
    private JTextField hostField;
    private JTextField portField;
    private JButton connectBtn, disconnectBtn;
    private JLabel statusLabel;

    // telemetry
    private JLabel timeLabel, speedLabel, batteryLabel, tempLabel;
    private JProgressBar batteryBar;

    // direction indicators (reemplazo de brújula)
    private JButton forwardIndicator, leftIndicator, rightIndicator;

    // commands
    private JButton speedUpBtn, slowDownBtn, turnLeftBtn, turnRightBtn;

    // authentication
    private final String adminPass = "admin123";

    public AdminClient() {
    setTitle("Telemetry Admin - Java");
    setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
    setLocationRelativeTo(null);
    setLayout(new BorderLayout());

    createTopPanel();
    createMainPanel();

    // Ajusta automáticamente al tamaño mínimo necesario
    pack();
    setMinimumSize(getSize()); // evita que el usuario achique más de lo debido
}


    private void createTopPanel() {
        JPanel top = new JPanel(new GridLayout(2, 4, 10, 5));
        top.setBorder(new EmptyBorder(10, 10, 10, 10));

        top.add(new JLabel("Server Host:"));
        hostField = new JTextField("127.0.0.1");
        top.add(hostField);

        top.add(new JLabel("Server Port:"));
        portField = new JTextField("5555");
        top.add(portField);

        connectBtn = new JButton("CONNECT");
        styleButton(connectBtn, new Color(39, 174, 96));
        connectBtn.addActionListener(e -> connect());
        top.add(connectBtn);

        disconnectBtn = new JButton("DISCONNECT");
        styleButton(disconnectBtn, new Color(192, 57, 43));
        disconnectBtn.setEnabled(false);
        disconnectBtn.addActionListener(e -> disconnect());
        top.add(disconnectBtn);

        statusLabel = new JLabel("Status: Disconnected");
        statusLabel.setForeground(Color.RED);
        top.add(statusLabel);

        add(top, BorderLayout.NORTH);
    }

    private void createMainPanel() {
        JPanel main = new JPanel(new GridLayout(1, 2, 15, 0));
        main.setBorder(new EmptyBorder(10, 10, 10, 10));

        // panel telemetry
        JPanel telemetry = new JPanel();
        telemetry.setLayout(new BoxLayout(telemetry, BoxLayout.Y_AXIS));
        telemetry.setBorder(BorderFactory.createTitledBorder("Vehicle Telemetry"));

        JPanel timePanel = createTelemetryBox("Time:", "");
        timeLabel = new JLabel(LocalTime.now().format(DateTimeFormatter.ofPattern("HH:mm:ss")));
        timeLabel.setFont(new Font("Segoe UI", Font.PLAIN, 14));
        timePanel.add(timeLabel, BorderLayout.EAST);
        telemetry.add(timePanel);

        // dirección como 3 botones indicadores
        JPanel dirIndicators = new JPanel(new FlowLayout(FlowLayout.CENTER, 15, 5));
        dirIndicators.setBorder(BorderFactory.createTitledBorder("Direction Indicator"));

        forwardIndicator = makeDirButton("↑ Forward");
        leftIndicator = makeDirButton("← Left");
        rightIndicator = makeDirButton("Right →");

        dirIndicators.add(leftIndicator);
        dirIndicators.add(forwardIndicator);
        dirIndicators.add(rightIndicator);

        telemetry.add(dirIndicators);

        JPanel speedPanel = createTelemetryBox("SPEED:", "0.0 km/h");
        speedLabel = (JLabel) speedPanel.getComponent(1);
        telemetry.add(speedPanel);

        JPanel batteryPanel = createTelemetryBox("BATTERY:", "100%");
        batteryLabel = (JLabel) batteryPanel.getComponent(1);
        telemetry.add(batteryPanel);

        batteryBar = new JProgressBar(0, 100);
        batteryBar.setValue(100);
        batteryBar.setForeground(new Color(46, 204, 113));
        batteryBar.setPreferredSize(new Dimension(200, 20));
        batteryBar.setStringPainted(true);
        batteryBar.setBorder(new LineBorder(Color.GRAY, 1));
        JPanel barWrapper = new JPanel(new FlowLayout(FlowLayout.CENTER));
        barWrapper.add(batteryBar);
        telemetry.add(barWrapper);

        JPanel tempPanel = createTelemetryBox("TEMP:", "0.0°C");
        tempLabel = (JLabel) tempPanel.getComponent(1);
        telemetry.add(tempPanel);

        telemetry.add(Box.createVerticalGlue());
        main.add(telemetry);

        // panel controles
        JPanel controls = new JPanel();
        controls.setLayout(new BoxLayout(controls, BoxLayout.Y_AXIS));
        controls.setBorder(BorderFactory.createTitledBorder("Controls"));
        controls.setBackground(new Color(245, 248, 255));

        // Speed control
        JPanel speedBox = new JPanel();
        speedBox.setLayout(new BoxLayout(speedBox, BoxLayout.Y_AXIS));
        speedBox.setBorder(new EmptyBorder(10, 10, 10, 10));
        speedBox.setBackground(new Color(245, 248, 255));

        JLabel speedTitle = new JLabel("Speed Control", SwingConstants.CENTER);
        speedTitle.setFont(new Font("Segoe UI", Font.BOLD, 14));
        speedTitle.setAlignmentX(Component.CENTER_ALIGNMENT);
        speedBox.add(speedTitle);
        speedBox.add(Box.createRigidArea(new Dimension(0, 10)));

        speedUpBtn = new JButton("↑ Speed Up");
        styleButton(speedUpBtn, new Color(39, 174, 96));
        speedUpBtn.setPreferredSize(new Dimension(180, 45));
        speedUpBtn.setMaximumSize(new Dimension(180, 45));
        speedUpBtn.setAlignmentX(Component.CENTER_ALIGNMENT);
        speedUpBtn.addActionListener(e -> sendCommand("CMD SPEED UP"));
        speedBox.add(speedUpBtn);

        speedBox.add(Box.createRigidArea(new Dimension(0, 10)));

        slowDownBtn = new JButton("↓ Slow Down");
        styleButton(slowDownBtn, new Color(230, 126, 34));
        slowDownBtn.setPreferredSize(new Dimension(180, 45));
        slowDownBtn.setMaximumSize(new Dimension(180, 45));
        slowDownBtn.setAlignmentX(Component.CENTER_ALIGNMENT);
        slowDownBtn.addActionListener(e -> sendCommand("CMD SLOW DOWN"));
        speedBox.add(slowDownBtn);

        controls.add(speedBox);
        controls.add(Box.createRigidArea(new Dimension(0, 20)));

        // Direction control
        JPanel dirBox = new JPanel();
        dirBox.setLayout(new BoxLayout(dirBox, BoxLayout.Y_AXIS));
        dirBox.setBorder(new EmptyBorder(10, 10, 10, 10));
        dirBox.setBackground(new Color(245, 248, 255));

        JLabel dirTitle = new JLabel("Direction Control", SwingConstants.CENTER);
        dirTitle.setFont(new Font("Segoe UI", Font.BOLD, 14));
        dirTitle.setAlignmentX(Component.CENTER_ALIGNMENT);
        dirBox.add(dirTitle);
        dirBox.add(Box.createRigidArea(new Dimension(0, 10)));

        JPanel dirPanel = new JPanel(new FlowLayout(FlowLayout.CENTER, 15, 5));
        dirPanel.setBackground(new Color(245, 248, 255));

        turnLeftBtn = new JButton("← Turn Left");
        styleButton(turnLeftBtn, new Color(52, 152, 219));
        turnLeftBtn.setPreferredSize(new Dimension(175, 75));
        turnLeftBtn.setMaximumSize(new Dimension(175, 75));
        turnLeftBtn.addActionListener(e -> sendCommand("CMD TURN LEFT"));

        turnRightBtn = new JButton("Turn Right →");
        styleButton(turnRightBtn, new Color(52, 152, 219));
        turnRightBtn.setPreferredSize(new Dimension(175, 75));
        turnRightBtn.setMaximumSize(new Dimension(175, 75));
        turnRightBtn.addActionListener(e -> sendCommand("CMD TURN RIGHT"));

        dirPanel.add(turnLeftBtn);
        dirPanel.add(turnRightBtn);
        dirBox.add(dirPanel);

        controls.add(dirBox);
        controls.add(Box.createVerticalGlue());
        main.add(controls);

        add(main, BorderLayout.CENTER);
    }

    private JPanel createTelemetryBox(String label, String initial) {
        JPanel panel = new JPanel(new BorderLayout());
        panel.setBorder(new EmptyBorder(5, 10, 5, 10));
        JLabel l = new JLabel(label);
        l.setFont(new Font("Segoe UI", Font.BOLD, 14));
        JLabel val = new JLabel(initial);
        val.setFont(new Font("Segoe UI", Font.PLAIN, 14));
        panel.add(l, BorderLayout.WEST);
        panel.add(val, BorderLayout.EAST);
        return panel;
    }

    private void styleButton(JButton btn, Color bg) {
        btn.setBackground(bg);
        btn.setForeground(Color.WHITE);
        btn.setFocusPainted(false);
        btn.setFont(new Font("Segoe UI", Font.BOLD, 14));
        btn.setBorder(BorderFactory.createEmptyBorder(8, 15, 8, 15));
    }

    private JButton makeDirButton(String text) {
        JButton btn = new JButton(text);
        btn.setPreferredSize(new Dimension(120, 60));
        btn.setEnabled(false); // solo indicador, no clickeable
        btn.setBackground(new Color(200, 200, 200)); // gris inicial
        btn.setForeground(Color.BLACK);
        btn.setFont(new Font("Segoe UI", Font.BOLD, 14));
        return btn;
    }

    private void updateDirectionButtons(String dir) {
        // reset
        forwardIndicator.setBackground(new Color(200, 200, 200));
        leftIndicator.setBackground(new Color(200, 200, 200));
        rightIndicator.setBackground(new Color(200, 200, 200));

        if (dir == null) return;
        dir = dir.toUpperCase();

        if (dir.equals("FORWARD") || dir.equals("F") || dir.equals("N")) {
            forwardIndicator.setBackground(new Color(39, 174, 96)); // verde
        } else if (dir.equals("LEFT") || dir.equals("L") || dir.equals("W")) {
            leftIndicator.setBackground(new Color(41, 128, 185)); // azul
        } else if (dir.equals("RIGHT") || dir.equals("R") || dir.equals("E")) {
            rightIndicator.setBackground(new Color(211, 84, 0)); // naranja
        }
    }

    // networking
    private void connect() {
        String host = hostField.getText().trim();
        int port;
        try {
            port = Integer.parseInt(portField.getText().trim());
        } catch (NumberFormatException ex) {
            JOptionPane.showMessageDialog(this, "Invalid port", "Error", JOptionPane.ERROR_MESSAGE);
            return;
        }

        connectBtn.setEnabled(false);
        new Thread(() -> {
            try {
                socket = new Socket();
                socket.connect(new InetSocketAddress(host, port), 5000);
                in = new BufferedReader(new InputStreamReader(socket.getInputStream()));
                out = new BufferedWriter(new OutputStreamWriter(socket.getOutputStream()));

                sendRaw(String.format("LOGIN ADMIN %s\n", adminPass));

                receiving = true;
                startReceiving();

                SwingUtilities.invokeLater(() -> {
                    statusLabel.setText("Status: Connected");
                    statusLabel.setForeground(new Color(0, 150, 0));
                    connectBtn.setEnabled(false);
                    disconnectBtn.setEnabled(true);
                });
            } catch (IOException ioe) {
                SwingUtilities.invokeLater(() -> {
                    JOptionPane.showMessageDialog(this, "No se pudo conectar: " + ioe.getMessage(),
                            "Error", JOptionPane.ERROR_MESSAGE);
                    connectBtn.setEnabled(true);
                });
                cleanup();
            }
        }).start();
    }

    private void disconnect() {
        receiving = false;
        sendCommandSync("QUIT");
        cleanup();
        SwingUtilities.invokeLater(() -> {
            statusLabel.setText("Status: Disconnected");
            statusLabel.setForeground(Color.RED);
            connectBtn.setEnabled(true);
            disconnectBtn.setEnabled(false);
        });
    }

    private void cleanup() {
        receiving = false;
        try {
            if (recvThread != null) recvThread.interrupt();
        } catch (Exception ignored) {}
        try {
            if (in != null) in.close();
        } catch (Exception ignored) {}
        try {
            if (out != null) out.close();
        } catch (Exception ignored) {}
        try {
            if (socket != null) socket.close();
        } catch (Exception ignored) {}
        in = null;
        out = null;
        socket = null;
    }

    private void startReceiving() {
        recvThread = new Thread(() -> {
            String line;
            try {
                while (receiving && socket != null && !socket.isClosed()) {
                    line = in.readLine();
                    if (line == null) break;
                    line = line.trim();
                    if (line.isEmpty()) continue;

                    if (line.startsWith("TLM")) {
                        String payload = line.substring(line.indexOf(' ') + 1);
                        HashMap<String, String> tel = parseKeyValues(payload, ";", "=");
                        updateTelemetry(tel);
                    } else {
                        System.out.println("SERVER: " + line);
                    }
                }
            } catch (IOException ignored) {}
        }, "recv-thread");
        recvThread.setDaemon(true);
        recvThread.start();
    }

    private HashMap<String, String> parseKeyValues(String s, String pairSep, String kvSep) {
        HashMap<String, String> map = new HashMap<>();
        if (s == null || s.isEmpty()) return map;
        for (String p : s.split(pairSep)) {
            String[] kv = p.split(kvSep, 2);
            if (kv.length == 2) map.put(kv[0].trim().toLowerCase(), kv[1].trim());
        }
        return map;
    }

    private void updateTelemetry(HashMap<String, String> tel) {
        SwingUtilities.invokeLater(() -> {
            if (tel.containsKey("speed"))
                speedLabel.setText(tel.get("speed") + " km/h");
            if (tel.containsKey("battery")) {
                try {
                    int b = Integer.parseInt(tel.get("battery"));
                    batteryLabel.setText(b + "%");
                    batteryBar.setValue(Math.max(0, Math.min(100, b)));
                    batteryBar.setForeground(b > 30 ? new Color(46, 204, 113) : Color.RED);
                } catch (NumberFormatException ignored) {}
            }
            if (tel.containsKey("temp"))
                tempLabel.setText(tel.get("temp") + "°C");
            if (tel.containsKey("dir"))
                updateDirectionButtons(tel.get("dir"));
            if (tel.containsKey("ts"))
                timeLabel.setText(tel.get("ts"));
        });
    }

    private void sendRaw(String msg) {
        new Thread(() -> {
            try {
                if (out != null) {
                    out.write(msg);
                    out.flush();
                }
            } catch (IOException ignored) {}
        }).start();
    }

    private void sendCommand(String cmd) {
        if (out == null) return;
        if (!cmd.endsWith("\n")) cmd += "\n";
        sendRaw(cmd);
    }

    private void sendCommandSync(String cmd) {
        if (out == null) return;
        try {
            if (!cmd.endsWith("\n")) cmd += "\n";
            out.write(cmd);
            out.flush();
        } catch (IOException ignored) {}
    }

    public static void main(String[] args) {
        SwingUtilities.invokeLater(() -> {
            AdminClient app = new AdminClient();
            app.setVisible(true);
        });
    }
}
