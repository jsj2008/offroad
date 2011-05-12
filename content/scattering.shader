<program>
  <attribute name="position" unit="0"></attribute>

  <shader type="vertex">
  <![CDATA[
  in vec3 position;

  uniform vec3 cam_pos;
  uniform vec3 light_dir;
  uniform vec3 inv_wave_length;
  uniform float cam_height;
  uniform float outer_radius;
  uniform float outer_radius_2;
  uniform float inner_radius;
  uniform float inner_radius_2;
  uniform float kr_e_sun;
  uniform float km_e_sun;
  uniform float kr_4pi;
  uniform float km_4pi;
  uniform float scale;
  uniform float scale_depth;
  uniform float scale_over_scale_depth;

  const int n_samples = 2;
  const float f_samples = 2.0;

  out vec3 v_dir;
  out vec3 sec_color;
  out vec3 fro_color;

  float get_scale(float angle) {
    float x = 1.0 - angle;
    return scale_depth * exp(-0.00287 + x*(0.459 + x*(3.83 + x*(-6.80 + x*5.25))));
  }

  void main(void) {
    gl_Position = gl_ModelViewProjectionMatrix * vec4(position, 1);
    vec3 pos = position;
    vec3 ray = pos - cam_pos;
    float far = length(ray);
    ray /= far;

    vec3 start = cam_pos;
    float height = length(start);
    float depth = exp(scale_over_scale_depth * (inner_radius - cam_height));
    float start_angle = dot(ray, start) / height;
    float start_offset = depth * get_scale(start_angle);

    float sample_length = far / f_samples;
    float scaled_length = sample_length * scale;
    vec3 sample_ray = ray * sample_length;
    vec3 sample_point = start + sample_ray * 0.5;

    vec3 front_color = vec3(0.0, 0.0, 0.0);
    for(int i = 0; i < n_samples; i++) {
      float height = length(sample_point);
      float depth = exp(scale_over_scale_depth * (inner_radius - height));
      float light_angle = dot(light_dir, sample_point) / height;
      float cam_angle = dot(ray, sample_point) / height;
      float scatter = (start_offset + depth*(get_scale(light_angle) - get_scale(cam_angle)));
      vec3 attenuate = exp(-scatter * (inv_wave_length * kr_4pi + km_4pi));
      front_color += attenuate * (depth * scaled_length);
      sample_point += sample_ray;
    }

    sec_color = front_color * km_e_sun;
    fro_color = front_color * (inv_wave_length * kr_e_sun);
    v_dir = cam_pos - pos;
  }
  ]]>
  </shader>

  <shader type="pixel">
  <![CDATA[
  uniform vec3 light_dir;
  uniform float g;
  uniform float g_2;

  in vec3 v_dir;
  in vec3 sec_color;
  in vec3 fro_color;

  void main (void) {
    float angle = dot(light_dir, v_dir) / length(v_dir);
    float mie_phase = 1.5 * ((1.0 - g_2) / (2.0 + g_2)) * (1.0 + angle*angle) / pow(1.0 + g_2 - 2.0*g*angle, 1.5);
    gl_FragColor = vec4(fro_color + mie_phase * sec_color, 1);
    /*gl_FragColor.a = gl_FragColor.b;*/
    /*gl_FragColor = vec4(sec_color, 1);*/
  }
  ]]>
  </shader>
</program>
