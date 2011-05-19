<program>
  <attribute name="position" unit="0"></attribute>

  <shader type="vertex">
  in vec3 position;
  out vec2 coords;
  
  void main() {
    gl_Position = gl_ModelViewProjectionMatrix * vec4(position, 1);
    coords = position.xy * 0.5 + 0.5;
  }
  </shader>

  <shader type="pixel">
  <![CDATA[
  in vec2 coords;
  uniform sampler2D color;
  uniform sampler2D depth;
  uniform mat4 previousModelView;
  uniform mat4 proj;
  uniform mat4 modelViewInverse;
  uniform mat4 projInverse;

  const int num_samples = 3;

  void main() {
    float zOverW = texture2D(depth, coords).r;
    vec4 h = vec4(coords.x * 2 - 1, (1 - coords.y) * 2 - 1, zOverW, 1);
    vec4 d = (modelViewInverse * projInverse * h);
    vec4 world_pos = d / d.w;
    vec4 current_pos = h;
    vec4 previous_pos = proj * previousModelView * world_pos;
    previous_pos /= previous_pos.w;
    vec2 velocity = ((current_pos - previous_pos) / 5.f).xz;

    vec4 fragment = texture2D(color, coords);
    vec2 ccoords = coords;
    ccoords += velocity;
    for (int i = 1; i < num_samples; ++i, ccoords += velocity) {
      fragment += texture2D(color, ccoords);
    }
    fragment /= num_samples;
    fragment.a = 1;
    gl_FragColor = fragment;

    /*gl_FragColor = texture2D(color, coords);*/
  }
  ]]>
  </shader>
</program>
