String sr04Test(String pinSCL, String pinSDA) {
  pinMode(pinSCL.toInt(), OUTPUT);
  pinMode(pinSDA.toInt(), INPUT);

  digitalWrite(pinSCL.toInt(), LOW);
  delayMicroseconds(2);
  digitalWrite(pinSCL.toInt(), HIGH);
  delayMicroseconds(10);
  digitalWrite(pinSCL.toInt(), LOW);
  long duration = pulseIn(pinSDA.toInt(), HIGH);
  float distance = duration * 0.034 / 2;

  Serial.print("Distance: ");
  Serial.print(distance);    
  Serial.println(" cm");

  return String(distance, 4); 
  
} 