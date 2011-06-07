<program>
  <attribute name="position" unit="0"></attribute>
  <attribute name="normal" unit="1"></attribute>

  <shader type="vertex">
  in vec3 position;
  in vec3 normal;
  out vec3 pnormal;
  uniform mat4 modelView;
  uniform mat4 model;
  uniform mat4 proj;

  void main() {
    gl_Position = proj * modelView * vec4(position, 1);
    pnormal = normalize((modelView * model * vec4(normal, 0)).xyz);
  }
  </shader>

  <shader type="pixel">
  in vec3 pnormal;
  uniform mat4 modelView;
  uniform vec3 light_dir;

  const vec3 light_diffuse = vec3(0.8, 0.8, 0.8);

  void main() {
    vec4 L = vec4(normalize((modelView * vec4(light_dir, 0)).xyz), 0);
    vec4 N = vec4(normalize(pnormal), 0);
    vec4 diffuse = vec4(max(dot(N, L), 0.0) * light_diffuse, 0);
    vec4 ambient = vec4(0.1, 0.1, 0.1, 1);

    gl_FragColor = diffuse;
    gl_FragColor.a = 0.5;
  }
  </shader>
</program>
