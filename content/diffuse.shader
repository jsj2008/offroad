<program>
  <attribute name="position" unit="0"></attribute>
  <attribute name="normal" unit="1"></attribute>

  <shader type="vertex">
  in vec3 position;
  in vec3 normal;
  
  out vec3 pnormal;
  
  void main() {
    gl_Position = gl_ModelViewProjectionMatrix * vec4(position, 1);
    pnormal = normal;
  }
  </shader>

  <shader type="pixel">
  in vec3 pnormal;
  
  void main() {
    gl_FragColor = vec4(pnormal,1);
  }
  </shader>
</program>
