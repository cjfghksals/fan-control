unsigned long timerEnd = 0;
bool timerActive = false;
String buf = "";

void setup() {
  Serial1.begin(9600);
  pinMode(9, OUTPUT);
  digitalWrite(9, LOW);
}

void loop() {
  while (Serial1.available()) {
    char c = Serial1.read();
    if (c == '\n') {
      buf.trim();
      handleCommand(buf);
      buf = "";
    } else {
      buf += c;
    }
  }

  if (timerActive && millis() >= timerEnd) {
    digitalWrite(9, LOW);
    timerActive = false;
  }
}

void handleCommand(String cmd) {
  if (cmd == "1") {
    digitalWrite(9, HIGH);
    timerActive = false;
  } else if (cmd == "0") {
    digitalWrite(9, LOW);
    timerActive = false;
  } else if (cmd.startsWith("T")) {
    unsigned long secs = cmd.substring(1).toInt();
    if (secs > 0) {
      digitalWrite(9, HIGH);
      timerEnd = millis() + secs * 1000UL;
      timerActive = true;
    }
  }
}
