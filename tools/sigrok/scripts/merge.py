import pandas as pd

df_data = pd.read_csv("data.csv")
df_clk = pd.read_csv("clk.csv")

n = min(len(df_data), len(df_clk))
merged = pd.DataFrame({
    "SDA": df_data.iloc[:n, 0].astype(int).values,
    "SCL": df_clk.iloc[:n, 0].astype(int).values,
})
merged.to_csv("merged_sda_scl.csv", index=False)
