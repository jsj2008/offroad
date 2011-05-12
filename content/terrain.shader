<program>
  <attribute name="position" unit="0"></attribute>
  <attribute name="normal" unit="1"></attribute>
  <attribute name="coords" unit="2"></attribute>

  <shader type="vertex">
  <![CDATA[
  in vec3 position;
  in vec3 normal;
  in vec2 coords;

  out vec3 pnormal;
  out vec2 pcoords;
  
  void main() {
    gl_Position = gl_ModelViewProjectionMatrix * vec4(position, 1);
    pnormal = normal;
    pcoords = coords * 100;
  }
  ]]>
  </shader>

  <shader type="pixel">
  in vec3 pnormal;
  in vec2 pcoords;

  uniform sampler2D grass;

  const vec3 light_dir = normalize(vec3(0, -0.4, 0.2));
  
  void main() {
    float diffuse = max(0, dot(-normalize(pnormal), light_dir));
    gl_FragColor = texture2D(grass, pcoords) * diffuse;
    gl_FragColor.a = 1;
  }
  </shader>
</program>
