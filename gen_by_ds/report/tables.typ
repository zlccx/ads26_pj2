#let tbl_dataset = table(
  columns: 3,
  align: (left, right, right),
  column-gutter: 6pt,
  stroke: 0.4pt + luma(150),
  strong("数据集"), strong("结点数"), strong("弧数"),
  ..((
    "NY", "264,346", "733,846",
    "BAY", "321,270", "800,172",
    "COL", "435,666", "1,057,066",
    "FLA", "1,070,376", "2,712,798",
    "NW", "1,207,945", "2,840,208",
    "NE", "1,524,453", "3,897,636",
    "CAL", "1,890,815", "4,657,742"
  )).flatten(),
)

#let tbl_links = table(
  columns: 4,
  align: (left, right, right, left),
  column-gutter: 6pt,
  stroke: 0.4pt + luma(150),
  strong("网络"), strong("结点数"), strong("弧数"), strong("下载链接（.gr.gz，物理距离版本）"),
  ..((
    "NY", "264,346", "733,846", link("http://www.diag.uniroma1.it/challenge9/data/USA-road-d/USA-road-d.NY.gr.gz"),
    "BAY", "321,270", "800,172", link("http://www.diag.uniroma1.it/challenge9/data/USA-road-d/USA-road-d.BAY.gr.gz"),
    "COL", "435,666", "1,057,066", link("http://www.diag.uniroma1.it/challenge9/data/USA-road-d/USA-road-d.COL.gr.gz"),
    "FLA", "1,070,376", "2,712,798", link("http://www.diag.uniroma1.it/challenge9/data/USA-road-d/USA-road-d.FLA.gr.gz"),
    "NW", "1,207,945", "2,840,208", link("http://www.diag.uniroma1.it/challenge9/data/USA-road-d/USA-road-d.NW.gr.gz"),
    "NE", "1,524,453", "3,897,636", link("http://www.diag.uniroma1.it/challenge9/data/USA-road-d/USA-road-d.NE.gr.gz"),
    "CAL", "1,890,815", "4,657,742", link("http://www.diag.uniroma1.it/challenge9/data/USA-road-d/USA-road-d.CAL.gr.gz")
  )).flatten(),
)

#let link_note = "下载页：" + link("http://www.diag.uniroma1.it/challenge9/download.shtml")

#let tbl_p2p = table(
  columns: 5,
  align: (left, right, right, right, right),
  column-gutter: 6pt,
  stroke: 0.4pt + luma(150),
  strong("数据集"), strong("二叉堆"), strong("二项堆"), strong("斐波那契堆"), strong("二叉堆(惰性)"),
  ..((
    "NY", "14.15", "31.61", "33.51", "12.37",
    "BAY", "15.88", "38.58", "39.69", "13.69",
    "COL", "24.25", "51.54", "54.04", "19.00",
    "FLA", "82.35", "138.99", "136.13", "53.81",
    "NW", "91.38", "155.88", "156.27", "59.86",
    "NE", "128.02", "205.49", "216.24", "86.16",
    "CAL", "152.60", "242.15", "257.37", "103.88"
  )).flatten(),
)

#let tbl_full = table(
  columns: 5,
  align: (left, right, right, right, right),
  column-gutter: 6pt,
  stroke: 0.4pt + luma(150),
  strong("数据集"), strong("二叉堆"), strong("二项堆"), strong("斐波那契堆"), strong("二叉堆(惰性)"),
  ..((
    "NY", "45.59", "69.28", "62.74", "22.67",
    "BAY", "40.13", "71.35", "77.45", "25.15",
    "COL", "62.97", "97.28", "102.11", "41.01",
    "FLA", "170.47", "251.26", "263.65", "102.97",
    "NW", "207.11", "299.09", "319.60", "125.73",
    "NE", "265.60", "400.27", "425.13", "173.53",
    "CAL", "323.30", "488.69", "527.82", "216.01"
  )).flatten(),
)

#let tbl_ops = table(
  columns: 7,
  align: (left, right, right, right, right, right, right),
  column-gutter: 6pt,
  stroke: 0.4pt + luma(150),
  strong("实现"), strong("extractMin"), strong("insert"), strong("decreaseKey"), strong("弧扫描"), strong("关键字比较"), strong("比较/确定结点"),
  ..((
    "二叉堆", "925,156", "925,791", "76,056", "2,292,205", "16,288,857", "17.61",
    "二项堆", "925,156", "925,791", "76,057", "2,292,205", "11,457,435", "12.38",
    "斐波那契堆", "925,156", "925,791", "76,056", "2,292,205", "12,382,861", "13.38",
    "二叉堆(惰性)", "1,001,161", "1,001,847", "0", "2,292,205", "17,721,445", "19.16"
  )).flatten(),
)

#let tbl_ops_full = table(
  columns: 6,
  align: (left, right, right, right, right, right),
  column-gutter: 6pt,
  stroke: 0.4pt + luma(150),
  strong("实现"), strong("extractMin"), strong("insert"), strong("decreaseKey"), strong("关键字比较"), strong("比较/确定结点"),
  ..((
    "二叉堆", "1,890,815", "1,890,814", "150,411", "33,143,021", "17.53",
    "二项堆", "1,890,815", "1,890,814", "150,412", "23,294,884", "12.32",
    "斐波那契堆", "1,890,815", "1,890,814", "150,412", "25,171,384", "13.31",
    "二叉堆(惰性)", "2,041,228", "2,041,227", "0", "35,966,390", "19.02"
  )).flatten(),
)

#let ratio_binom = "1.91"
#let ratio_fib = "1.98"
#let ratio_lazy = "0.74"
#let n_queries = "1000"
#let n_datasets = "7"
#let largest_net = "CAL"
#let nodes_min = "264,346"
#let nodes_max = "1,890,815"
#let arcs_max = "4,657,742"
#let size_ratio = "7.2"
#let dec_per_pop = "0.082"
#let dec_inv = "12.2"
#let settled_per_query = "925,156"
#let arcs_per_query = "2,292,205"
#let lazy_pop_ratio = "1.08"
#let full_dec_per_pop = "0.080"
#let cmp_bin = "17.6"
#let cmp_binom = "12.4"
#let cmp_fib = "13.4"
#let cmp_lazy = "17.7"
