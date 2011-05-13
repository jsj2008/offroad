<program>
  <attribute name="position" unit="0"></attribute>
  <attribute name="normal" unit="1"></attribute>

  <shader type="vertex">
  in vec3 position;
  in vec3 normal;
  in vec2 color;
  in vec2 ambient;
  out vec3 pnormal;
  out vec2 pcolor;
  out vec2 pambient;
  uniform mat4 modelView;
  uniform mat4 proj;
  
  void main() {
    gl_Position = proj * modelView * vec4(position, 1);
    pnormal = normal;
    pcolor = color;
    pambient = ambient;
  }
  </shader>

  <shader type="pixel">
  in vec3 pnormal;
  in vec2 pcolor;
  in vec2 pambient;
  uniform sampler2D texture0;
  uniform sampler2D texture1;
 
  void main() {
    gl_FragColor = texture2D(texture0, pcolor) * texture2D(texture1, pambient).r;
  }
  </shader>
</program>
