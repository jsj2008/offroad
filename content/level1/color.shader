<program>
  <attribute name="position" unit="0"></attribute>
  <attribute name="normal" unit="1"></attribute>
  <attribute name="color" unit="2"></attribute>

  <shader type="vertex">
  <![CDATA[
  in vec3 position;
  in vec3 normal;
  in vec2 color;
  out vec3 pnormal;
  out vec2 pcolor;
  uniform mat4 modelView;
  uniform mat4 proj;
  
  void main() {
    gl_Position = proj * modelView * vec4(position, 1);
    pnormal = (modelView * vec4(normal, 0)).xyz;
    pcolor = color;
  }
  ]]>
  </shader>

  <shader type="pixel">
  <![CDATA[
  in vec3 pnormal;
  in vec2 pcolor;
  uniform sampler2D texture0;
  uniform mat4 modelView;
  uniform vec3 light_dir;
 
  const vec3 light_diffuse = vec3(0.8, 0.8, 0.8);

  void main() {
    vec4 L = vec4(normalize((modelView * vec4(light_dir, 0)).xyz), 0);
    vec4 N = vec4(normalize(pnormal), 0);
    vec4 diffuse = vec4(max(-dot(N, L), 0.0) * light_diffuse, 0);
    vec4 frag_color = texture2D(texture0, pcolor);
    gl_FragColor = diffuse;
  }
  ]]>
  </shader>
</program>
