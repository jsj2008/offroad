<program>
  <attribute name="position" unit="0"></attribute>
  <attribute name="normal" unit="1"></attribute>
  <attribute name="normal" unit="1"></attribute>

  <shader type="vertex">
  in vec3 position;
  in vec3 normal;
  in vec2 coords;
  out vec3 pnormal;
  out vec2 pcoords;
  uniform mat4 modelView;
  uniform mat4 model;
  uniform mat4 proj;

  void main() {
    gl_Position = proj * modelView * vec4(position, 1);
    pnormal = normalize((modelView * model * vec4(normal, 0)).xyz);
    pcoords = coords;
  }
  </shader>

  <shader type="pixel">
  in vec3 pnormal;
  in vec2 pcoords;
  uniform sampler2D texture0;
  uniform mat4 modelView;
  uniform vec3 light_dir;
 
  void main() {
    vec4 base = texture2D(texture0, pcoords);
    vec4 L = vec4(normalize((modelView * vec4(light_dir, 0)).xyz), 0);
    vec4 N = vec4(normalize(pnormal), 0);

    gl_FragColor = min(max(dot(N, L), 0.0)+0.2, 1) * base;
    gl_FragColor.a = 1;
  }
  </shader>
</program>
