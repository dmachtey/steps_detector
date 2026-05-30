# -*- coding: utf-8 -*-
"""
Generated script for ESP-DSP optimization of Window.c
"""
import re

def esp_dsp_kernel(n_features=12):
    """
    Post-procesa Classifier.c para reemplazar el loop de compute_kernel
    con dsps_dotprod_f32 de la librería ESP-DSP.

    La optimización:
      - Agrega #include "esp_dsp.h" en Classifier.c
      - Los valores del vector soporte llegan como doubles via va_arg;
        se copian primero a un arreglo float[] y luego se llama
        dsps_dotprod_f32(x, sv, &kernel, n_features) que utiliza
        instrucciones SIMD del Xtensa LX6/LX7 del ESP32.

    INPUT:
    --------
    n_features : int, cantidad de características del modelo (default 12 para PCP)

    OUTPUT:
    --------
    Sobreescribe Classifier.c con la versión optimizada.
    """
    with open("Classifier.c", "r") as f:
        content = f.read()

    # 1. Agregar include de ESP-DSP después del include de Classifier.h
    content = content.replace(
        '#include "Classifier.h"',
        '#include "Classifier.h"\n#include "esp_dsp.h"'
    )

    # 2. Corregir va_start: el 2do argumento debe ser el ultimo param nombrado (x),
    #    no el número de features. micromlgen a veces genera va_start(w, N).
    content = re.sub(
        r'va_start\(w,\s*\d+\)',
        'va_start(w, x)',
        content
    )

    # 3. Reemplazar el loop acumulador por dsps_dotprod_f32
    #    Patron buscado (generado por micromlgen para kernel lineal):
    #      for (uint16_t i = 0; i < N; i++) {
    #          kernel += x[i] * va_arg(w, double);
    #      }
    old_loop = (
        r'for \(uint16_t i = 0; i < \d+; i\+\+\) \{\s*'
        r'kernel \+= x\[i\] \* va_arg\(w, double\);\s*'
        r'\}'
    )
    new_loop = (
        f'float sv[{n_features}];\n'
        f'    for (uint16_t i = 0; i < {n_features}; i++) {{\n'
        f'        sv[i] = (float)va_arg(w, double);\n'
        f'    }}\n'
        f'    va_end(w);\n'
        f'    dsps_dotprod_f32(x, sv, &kernel, {n_features});'
    )
    content, n_subs = re.subn(old_loop, new_loop, content)

    if n_subs == 0:
        print("[esp_dsp_kernel] AVISO: no se encontro el patron del loop "
              "en compute_kernel. Verificar que el kernel es lineal.")
    else:
        print(f"[esp_dsp_kernel] compute_kernel reemplazado con "
              f"dsps_dotprod_f32 (n_features={n_features}).")

    with open("Classifier.c", "w") as f:
        f.write(content)


def process_classifier(code, type_clf):
    # Standard Classifier generation
    with open("Classifier.c", "w") as file:
        file.write('#include "Classifier.h"\n\n')
    x = code.split("protected:")
    index_public = x[0].find("public:")
    index_protected = x[1].find("protected:")
    index_protected_end = x[1].find("};")
    for part in [x[0][index_public+7:], x[1][index_protected+10:index_protected_end]]:
        pretty = []
        indent = 0
        for line in part.split('\n'):
            line = line.strip()
            if not line: continue
            if line[-1] in ['}', '};']: indent -= 1
            pretty.append(('    ' * indent) + line)
            if line[-1] == '{' or line == 'public:': indent += 1
        pretty_str = '\n'.join(pretty)
        pretty_str = re.sub(r'([;])\n(\s*?)(for|return|if) ', lambda m: '%s\n\n%s%s ' % m.groups(), pretty_str)
        pretty_str = re.sub(r'}\n', '}\n\n', pretty_str)
        pretty_str = re.sub(r'\}\n\n(\s*?)\}', lambda m: '}\n%s}' % m.groups(), pretty_str)
        pretty_str = re.sub(r',\s*\}', '}', pretty_str)
        with open("Classifier.c", "a") as file:
            file.write(pretty_str)
    
    with open("Classifier.h", "w") as file:
        file.write('#ifndef _INC_MODEL_H_\n#define _INC_MODEL_H_\n\n#include <stdarg.h>\n#include <stdint.h>\n\nint predict(float *x);\nconst char* idxToLabel(uint8_t classIdx);\nconst char* predictLabel(float *x);\n')
        if type_clf == 'GaussianNB': file.write('float gauss(float *x, float *theta, float *sigma);\n\n')
        if type_clf in ['RandomForest', 'SVM']: file.write('float compute_kernel(float *x, ...);\n\n')
        file.write('#endif')

    # Para SVM: optimizar compute_kernel con ESP-DSP
    if type_clf == 'SVM':
        # Detectar n_features del código generado
        m = re.search(r'for \(uint16_t i = 0; i < (\d+); i\+\+\)', x[1])
        n_features = int(m.group(1)) if m else 12
        esp_dsp_kernel(n_features=n_features)

def process_window(code):
    """
    Optimized Window.c generation with ESP-DSP primitives
    """
    x = code.split("protected:")
    index_public = x[0].find("public:")
    index_protected = x[1].find("protected:")
    index_protected_end = x[1].find("};")
    
    parts = [x[1][index_protected+10:index_protected_end], x[0][index_public+7:]]
    final_code_parts = []
    
    for part in parts:
        pretty = []
        indent = 0
        for line in part.split('\n'):
            line = line.strip()
            if not line: continue
            if line[-1] in ['}', '};']: indent -= 1
            pretty.append(('    ' * indent) + line)
            if line[-1] == '{' or line == 'public:': indent += 1
        p_str = '\n'.join(pretty)
        p_str = re.sub(r'([;])\n(\s*?)(for|return|if) ', lambda m: '%s\n\n%s%s ' % m.groups(), p_str)
        p_str = re.sub(r'}\n', '}\n\n', p_str)
        p_str = re.sub(r'= NULL;', ';', p_str)
        p_str = re.sub(r'\}\n\n(\s*?)\}', lambda m: '}\n%s}' % m.groups(), p_str)
        p_str = re.sub(r',\s*\}', '}', p_str)
        final_code_parts.append(p_str)
    
    content = '\n'.join(final_code_parts)
    
    # DSP OPTIMIZATIONS
    content = content.replace("abs(", "fabsf(")
    
    # Inject constants and DSP calls
    content = re.sub(r'for \(uint16_t j = 0; j < 4; j\+\+\) \{', 
                     r'''static const float dsp_mean_factor = 1.0f / 20.0f;
    for (uint16_t j = 0; j < 4; j++) {
        float mean = 0;
        dsps_dotprode_f32(&queue[j], &dsp_mean_factor, &mean, 20, 4, 0);

        float sum_sq = 0;
        dsps_dotprode_f32(&queue[j], &queue[j], &sum_sq, 20, 4, 4);
        float variance = (sum_sq / 20.0f) - (mean * mean);
        float std = sqrtf(variance);''', content)
    
    # Clean up redundant code
    content = re.sub(r'float mean = m;', '', content)
    content = re.sub(r'float std = 0(\.0+)?;', '', content)
    content = re.sub(r'mean \+= xi;', '', content)
    content = re.sub(r'std \+= x0 \* x0;', '', content)
    content = re.sub(r'mean /= 20;', '', content)
    content = re.sub(r'std = sqrt\(std / 20\);', '', content)
    
    with open("Window.c", "w") as f:
        f.write('#include "Window.h"\n\n' + content)
    
    with open("Window.h", "w") as file:
        file.write('#ifndef _INC_WINDOW_H_\n#define _INC_WINDOW_H_\n\n#include <stdarg.h>\n#include <stdint.h>\n#include <string.h>\n#include <stdbool.h>\n#include "math.h"\n#include "esp_dsp.h"\n\nbool transform(float *x, float *dest);\nvoid clear(void);\n#endif')

    with open("Window.c", "r") as f:
        lines = f.readlines()
    
    f_vec = ""; t_func = ""
    for line in lines:
        if "float features[" in line: f_vec = line
        if "bool transform" in line: t_func = line
            
    with open("Window.c", "w") as f:
        for line in lines:
            if line == t_func:
                f.write("bool transform(float *x, float *dest){\n")
            else:
                f.write(line)
    
def process_pca(code):
    import re
    # Extract mean vector
    mean_match = re.search(r'static float mean\[\] = \{(.*?)\};', code, re.DOTALL)
    mean_values = mean_match.group(1).strip() if mean_match else ""
    
    # Extract number of features (dimension)
    n_features = 32
    n_features_match = re.search(r'va_start\(w, (\d+)\);', code)
    if n_features_match:
        n_features = int(n_features_match.group(1))
        
    # Extract dot product weights (components)
    dot_calls = re.findall(r'u\[(\d+)\] = dot\(x,\s*(.*?)\);', code, re.DOTALL)
    dot_calls.sort(key=lambda x: int(x[0]))
    components = [call[1].strip() for call in dot_calls]
    n_components = len(components)

    # Source content
    c_content = f'#include "Pca.h"\n\n'
    c_content += f'static const float mean[{n_features}] = {{\n    {mean_values}\n}};\n'
    
    for i, comp in enumerate(components):
        c_content += f'\nstatic const float weights{i}[{n_features}] = {{\n    {comp}\n}};\n'
    
    c_content += f"""
/**
* Apply dimensionality reduction using ESP-DSP
*/
void transform(float *x, float *dest) {{
    static float x_centered[{n_features}];
    static float u[{n_components}];

    // Center data: x_centered = x - mean
    dsps_sub_f32(x, (float*)mean, x_centered, {n_features});

    // Project to principal components
"""
    for i in range(n_components):
        c_content += f"    dsps_dotprod_f32(x_centered, (float*)weights{i}, &u[{i}], {n_features});\n"
        
    c_content += f"""
    // Copy result back
    memcpy(dest != NULL ? dest : x, u, sizeof(float) * {n_components});
}}
"""

    with open("Pca.c", "w") as f:
        f.write(c_content)
        
    # Header content
    h_content = f"""#ifndef _INC_PCA_H_
#define _INC_PCA_H_

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "math.h"
#include "esp_dsp.h"

void transform(float *x, float *dest);

#endif
"""
    with open("Pca.h", "w") as f:
        f.write(h_content)


def iir_sos_header(fname_out, SOS_mat):
    """
    Write IIR SOS Header Files
    File format is compatible with CMSIS-DSP IIR 
    Directform II Filter Functions
    
    Mark Wickert March 2015-October 2016
    
    INPUT:
    --------
    fname_out: nombre del archivo
    SOS_mat: matriz SOS de coeficientes
    ------------------------
    OUTPUT:
    --------
    Archivo procesado *.h    
    """
    Ns, Mcol = SOS_mat.shape
    f = open(fname_out, 'wt')
    f.write('//define a IIR SOS CMSIS-DSP coefficient array\n\n')
    f.write('#include <stdint.h>\n\n')
    f.write('#include "arm_math.h"\n\n')
    f.write('#ifndef STAGES\n')
    f.write('#define STAGES %d\n' % Ns)
    f.write('#endif\n')
    f.write('/*********************************************************/\n');
    f.write('/*                     IIR SOS Filter Coefficients       */\n');
    f.write('float32_t ba_coeff[%d] = { //b0,b1,b2,a1,a2,... by stage\n' % (5 * Ns))
    for k in range(Ns):
        if (k < Ns - 1):
            f.write('    %+-13e, %+-13e, %+-13e,\n' % \
                    (SOS_mat[k, 0], SOS_mat[k, 1], SOS_mat[k, 2]))
            f.write('    %+-13e, %+-13e,\n' % \
                    (-SOS_mat[k, 4], -SOS_mat[k, 5]))
        else:
            f.write('    %+-13e, %+-13e, %+-13e,\n' % \
                    (SOS_mat[k, 0], SOS_mat[k, 1], SOS_mat[k, 2]))
            f.write('    %+-13e, %+-13e\n' % \
                    (-SOS_mat[k, 4], -SOS_mat[k, 5]))
    # for k in range(Ns):
    #     if (k < Ns-1):
    #         f.write('    %15.12f, %15.12f, %15.12f,\n' % \
    #                 (SOS_mat[k,0],SOS_mat[k,1],SOS_mat[k,2]))
    #         f.write('    %15.12f, %15.12f,\n' % \
    #                 (-SOS_mat[k,4],-SOS_mat[k,5]))
    #     else:
    #         f.write('    %15.12f, %15.12f, %15.12f,\n' % \
    #                 (SOS_mat[k,0],SOS_mat[k,1],SOS_mat[k,2]))
    #         f.write('    %15.12f, %15.12f\n' % \
    #                 (-SOS_mat[k,4],-SOS_mat[k,5]))
    f.write('};\n')
    f.write('/*********************************************************/\n')
    f.close() 

