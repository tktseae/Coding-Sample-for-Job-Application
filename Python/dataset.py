import torch
from torch.utils.data import Dataset

# DataLoader
class SentimentAnalysisDataset(Dataset):
  def __init__(self, dataframe, tokenizer, max_len):
    self.dataframe = dataframe
    self.dataframe["text"] = self.dataframe["text"].str.replace("\n", " ")
    self.tokenizer = tokenizer
    self.max_len = max_len

  def __len__(self):
    return len(self.dataframe)

  def __getitem__(self, idx):
    sample = self.dataframe.loc[idx]

    inputs = self.tokenizer.encode_plus(
      sample["text"],
      None,
      add_special_tokens=True,
      max_length=self.max_len,
      padding='max_length',
      return_token_type_ids=True
    )

    return {
      # "id": sample["id"],
      # "text": sample["text"],
      "label": sample["label"] - 1,
      "input_ids": torch.tensor(inputs["input_ids"], dtype=torch.long),
      "attention_mask": torch.tensor(inputs["attention_mask"], dtype=torch.long),
      "token_type_ids": torch.tensor(inputs["token_type_ids"], dtype=torch.long),
    }