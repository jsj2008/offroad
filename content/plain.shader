<program>
  <attribute name="position" unit="0"></attribute>
  <attribute name="normal" unit="1"></attribute>

  <shader type="vertex">
  in vec3 position;
  in vec3 normal;
  uniform mat4 modelView;
  uniform mat4 proj;
  
  void main() {
    gl_Position = proj * modelView * vec4(position, 1);
  }
  </shader>

  <shader type="pixel">
 
  void main() {
    gl_FragColor = vec4(1,1,1,1);
  }
  </shader>
</program>
