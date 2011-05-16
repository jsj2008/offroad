<program>
  <attribute name="position" unit="0"></attribute>
  <attribute name="normal" unit="1"></attribute>
  <attribute name="coords" unit="2"></attribute>

  <shader type="vertex">
  in vec3 position;
  in vec3 normal;
  in vec2 coords;
  out vec2 pcoords;
  out vec3 pnormal;
  uniform mat4 proj;
  uniform mat4 modelView;
  
  void main() {
    gl_Position = proj * modelView * vec4(position, 1);
    pcoords = coords;
    pnormal = normal;
  }
  </shader>

  <shader type="pixel">
  in vec3 pnormal;
  in vec2 pcoords;
  uniform sampler2D texture;
  
  void main() {
    gl_FragColor = texture2D(texture, pcoords);
  }
  </shader>
</program>
