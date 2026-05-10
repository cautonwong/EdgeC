import fitz  # PyMuPDF
import os

def pdf_to_images(pdf_path, output_folder, zoom_x=2.0, zoom_y=2.0):
    # 1. 检查并创建输出文件夹
    if not os.path.exists(output_folder):
        os.makedirs(output_folder)
        
    # 2. 打开 PDF
    doc = fitz.open(pdf_path)
    print(f"该文件共有 {doc.page_count} 页")

    # 3. 设置变换矩阵（用于提高分辨率）
    # 默认是 72 DPI，设置 zoom=2.0 相当于 144 DPI (2倍清晰)，zoom=3.0 为 216 DPI
    mat = fitz.Matrix(zoom_x, zoom_y)

    # 4. 遍历每一页
    for page in doc:
        # 渲染页面为位图 (PixMap)
        pix = page.get_pixmap(matrix=mat)
        
        # 生成文件名 (例如: page_001.png)
        filename = f"page_{page.number + 1:03d}.png"
        filepath = os.path.join(output_folder, filename)
        
        # 保存图片
        pix.save(filepath)
        print(f"已保存: {filepath}")

    doc.close()
    print("全部转换完成！")

# 使用示例
# zoom=2 代表放大2倍，越高越清晰，但文件越大
pdf_to_images("698.pdf", "698_images", zoom_x=2.0, zoom_y=2.0)