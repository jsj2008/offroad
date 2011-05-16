<program>
  <attribute name="position" unit="0"></attribute>
  <attribute name="normal" unit="1"></attribute>

  <shader type="vertex">
  in vec3 position;
  in vec3 normal;
  in vec2 color;
  out vec3 pnormal;
  out vec2 pcolor;
  uniform mat4 modelView;
  uniform mat4 proj;
  uniform mat4 mvInvTrans;
  
  void main() {
    gl_Position = proj * modelView * vec4(position, 1);
    pnormal = normal;
    pcolor = color;
  }
  </shader>

  <shader type="pixel">
  in vec3 pnormal;
  in vec2 pcolor;
  uniform sampler2D texture0;
 
  void main() {
    gl_FragColor = texture2D(texture0, pcolor);
  }
  </shader>
</program>
