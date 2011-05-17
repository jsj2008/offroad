<program>
  <attribute name="position" unit="0"></attribute>
  <attribute name="normal" unit="1"></attribute>
  <attribute name="color" unit="2"></attribute>

  <shader type="vertex">
  <![CDATA[
  in vec3 position;
  in vec3 normal;
  in vec2 color;
  out vec2 pcolor;
  uniform mat4 modelView;
  uniform mat4 proj;
  
  void main() {
    gl_Position = proj * modelView * vec4(position, 1);
    pcolor = color;
  }
  ]]>
  </shader>

  <shader type="pixel">
  <![CDATA[
  in vec2 pcolor;
  uniform sampler2D texture0;
 
  void main() {
    gl_FragColor = texture2D(texture0, pcolor);
  }
  ]]>
  </shader>
</program>
