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
  out vec3 reflected;
  out float height;
  uniform mat4 modelView;
  uniform mat4 model;
  uniform mat4 proj;
  uniform vec3 cam_pos;

  void main() {
    gl_Position = proj * modelView * vec4(position, 1);
    pnormal = normalize((modelView * model * vec4(normal, 0)).xyz);
    pcoords = coords;
    height = position.z;
    vec4 world_pos = model * vec4(position, 1);
    vec3 E = normalize(world_pos.xyz - cam_pos);
    reflected = reflect(E, pnormal);
  }
  </shader>

  <shader type="pixel">
  in float height;
  in vec2 pcoords;
  in vec3 pnormal;
  in vec3 reflected;
  uniform sampler2D texture0;
  uniform samplerCube sky;
  uniform mat4 modelView;
  uniform vec3 light_dir;
 
  const float reflect_factor = 0.3;

  void main() {
    vec4 L = vec4(normalize((modelView * vec4(light_dir, 0)).xyz), 0);
    vec4 N = vec4(normalize(pnormal), 0);
    float diffuse = min(max(dot(N, L), 0.0)+0.1, 1);

    vec4 base = texture2D(texture0, pcoords);
    base = mix(base, vec4(1-height, height, 0,0), 1-base.a);
    vec3 cube_color = textureCube(sky, -reflected).rgb;
    gl_FragColor = vec4(mix(base.rgb, cube_color, reflect_factor), 0.5) * diffuse;
  }
  </shader>
</program>
